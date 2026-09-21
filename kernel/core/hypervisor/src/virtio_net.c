/*
 * ATOMS OS — VirtIO Network Device Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#include "kernel/core/hypervisor/include/virtio_net.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

static void net_queue_notify_cb(VirtIODevice *dev, uint32_t q_idx) {
    if (!dev) return;
    VirtIONet *net = (VirtIONet *)dev->backend_data;
    if (!net) return;

    if (q_idx == VIRTIO_NET_QUEUE_TX) {
        virtio_net_process_tx(net);
    } else if (q_idx == VIRTIO_NET_QUEUE_RX) {
        virtio_net_flush_rx(net);
    }
}

static void net_reset_cb(VirtIODevice *dev) {
    if (!dev) return;
    VirtIONet *net = (VirtIONet *)dev->backend_data;
    if (net) {
        net->rx_head = 0;
        net->rx_tail = 0;
        net->rx_count = 0;
    }
}

VirtIONet *virtio_net_create(const uint8_t mac[6]) {
    VirtIONet *net = (VirtIONet *)kmalloc(sizeof(VirtIONet));
    if (!net) return NULL;
    memset(net, 0, sizeof(VirtIONet));

    if (mac) {
        memcpy(net->mac, mac, 6);
    } else {
        /* Default QEMU/KVM OUI style MAC: 52:54:00:12:34:56 */
        net->mac[0] = 0x52; net->mac[1] = 0x54; net->mac[2] = 0x00;
        net->mac[3] = 0x12; net->mac[4] = 0x34; net->mac[5] = 0x56;
    }

    net->link_status = VIRTIO_NET_S_LINK_UP;

    /* Create underlying VirtIODevice (Device ID = 1 for Net, 2 Queues: RX=0, TX=1) */
    net->base = virtio_device_create(VIRTIO_DEV_ID_NET, VIRTIO_PCI_DEVICE_NET, 2, sizeof(virtio_net_config_t));
    if (!net->base) {
        kfree(net);
        return NULL;
    }

    net->base->backend_data = net;
    net->base->on_queue_notify = net_queue_notify_cb;
    net->base->on_reset = net_reset_cb;

    /* Configure Supported Host Features */
    net->base->host_features = VIRTIO_NET_F_MAC | VIRTIO_NET_F_STATUS | VIRTIO_F_VERSION_1;

    /* Populate Configuration Space */
    virtio_net_config_t *cfg = (virtio_net_config_t *)net->base->config_space;
    memcpy(cfg->mac, net->mac, 6);
    cfg->status = net->link_status;
    cfg->max_virtqueue_pairs = 1;
    cfg->mtu = 1500;

    return net;
}

void virtio_net_destroy(VirtIONet *net) {
    if (!net) return;

    if (net->base) {
        virtio_device_destroy(net->base);
        net->base = NULL;
    }

    kfree(net);
}

void virtio_net_process_tx(VirtIONet *net) {
    if (!net || !net->base || !net->base->vm || !net->base->vm->guest_mem) return;

    VirtIODevice *dev = net->base;
    VirtQueue *vq = dev->queues[VIRTIO_NET_QUEUE_TX];
    GuestMemory *mem = dev->vm->guest_mem;

    VirtQueueChain chain;
    while (virtio_queue_pop_chain(vq, mem, &chain)) {
        if (chain.count < 1) continue;

        /* Parse header and packet chunks */
        uint32_t packet_bytes = 0;
        uint8_t packet_buf[VIRTIO_NET_MAX_PACKET_LEN];

        for (uint32_t i = 0; i < chain.count; i++) {
            VirtQueueBuffer *buf = &chain.buffers[i];
            if (!buf->hva || buf->is_write) continue;

            if (i == 0 && buf->len >= sizeof(virtio_net_hdr_t)) {
                /* Skip VirtIO Net Header in the first buffer */
                uint32_t payload_in_first = buf->len - sizeof(virtio_net_hdr_t);
                if (payload_in_first > 0 && (packet_bytes + payload_in_first) <= VIRTIO_NET_MAX_PACKET_LEN) {
                    memcpy(packet_buf + packet_bytes, (uint8_t *)buf->hva + sizeof(virtio_net_hdr_t), payload_in_first);
                    packet_bytes += payload_in_first;
                }
            } else {
                if ((packet_bytes + buf->len) <= VIRTIO_NET_MAX_PACKET_LEN) {
                    memcpy(packet_buf + packet_bytes, buf->hva, buf->len);
                    packet_bytes += buf->len;
                }
            }
        }

        if (packet_bytes > 0) {
            net->tx_packets++;
            net->tx_bytes += packet_bytes;

            /* In Phase 3 synthetic test / loopback, echo back to RX pool */
            virtio_net_inject_rx_packet(net, packet_buf, packet_bytes);
        }

        /* Complete TX chain */
        virtio_queue_complete_chain(vq, mem, chain.head_index, 0);

        /* Trigger TX Interrupt */
        virtio_device_raise_interrupt(dev, 0x01);
    }
}

bool virtio_net_inject_rx_packet(VirtIONet *net, const uint8_t *packet, uint32_t len) {
    if (!net || !packet || len == 0 || len > VIRTIO_NET_MAX_PACKET_LEN) return false;

    if (net->rx_count >= VIRTIO_NET_PACKET_QUEUE_DEPTH) {
        net->dropped_packets++;
        return false; /* Queue Full */
    }

    VirtIONetPacket *pkt = &net->rx_pool[net->rx_tail];
    memcpy(pkt->data, packet, len);
    pkt->len = len;
    pkt->valid = true;

    net->rx_tail = (net->rx_tail + 1) % VIRTIO_NET_PACKET_QUEUE_DEPTH;
    net->rx_count++;

    /* Attempt to push immediately into guest RX virtqueue */
    virtio_net_flush_rx(net);
    return true;
}

void virtio_net_flush_rx(VirtIONet *net) {
    if (!net || !net->base || !net->base->vm || !net->base->vm->guest_mem) return;

    VirtIODevice *dev = net->base;
    VirtQueue *vq = dev->queues[VIRTIO_NET_QUEUE_RX];
    GuestMemory *mem = dev->vm->guest_mem;

    while (net->rx_count > 0 && virtio_queue_has_available(vq, mem)) {
        VirtQueueChain chain;
        if (!virtio_queue_pop_chain(vq, mem, &chain)) {
            break;
        }

        VirtIONetPacket *pkt = &net->rx_pool[net->rx_head];
        if (!pkt->valid) break;

        uint32_t bytes_written = 0;
        uint32_t pkt_offset = 0;

        /* Write VirtIO Net Header + Payload into guest writable buffers */
        for (uint32_t i = 0; i < chain.count; i++) {
            VirtQueueBuffer *buf = &chain.buffers[i];
            if (!buf->hva || !buf->is_write) continue;

            uint8_t *dst = (uint8_t *)buf->hva;
            uint32_t space = buf->len;

            if (i == 0) {
                /* Write clean zeroed virtio_net_hdr_t */
                virtio_net_hdr_t hdr;
                memset(&hdr, 0, sizeof(hdr));
                uint32_t hdr_copy = (space >= sizeof(hdr)) ? sizeof(hdr) : space;
                memcpy(dst, &hdr, hdr_copy);
                dst += hdr_copy;
                space -= hdr_copy;
                bytes_written += hdr_copy;
            }

            if (space > 0 && pkt_offset < pkt->len) {
                uint32_t to_copy = pkt->len - pkt_offset;
                if (to_copy > space) to_copy = space;
                memcpy(dst, pkt->data + pkt_offset, to_copy);
                pkt_offset += to_copy;
                bytes_written += to_copy;
            }
        }

        /* Retire packet from internal ring */
        pkt->valid = false;
        net->rx_head = (net->rx_head + 1) % VIRTIO_NET_PACKET_QUEUE_DEPTH;
        net->rx_count--;

        net->rx_packets++;
        net->rx_bytes += bytes_written;

        /* Complete RX chain */
        virtio_queue_complete_chain(vq, mem, chain.head_index, bytes_written);

        /* Trigger RX Interrupt */
        virtio_device_raise_interrupt(dev, 0x01);
    }
}
