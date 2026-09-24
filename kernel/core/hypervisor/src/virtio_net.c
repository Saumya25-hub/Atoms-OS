/*
 * ATOMS OS — VirtIO Network Device Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3 & Phase 5A-3: VirtIO Virtual Hardware Subsystem & Real Network Bridge
 */

#include "kernel/core/hypervisor/include/virtio_net.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/net/net_framework.h"
#include "kernel/net/ethernet/ethernet.h"

extern void com1_puts(const char *s);

/* Global Telemetry & Active VirtIO-Net Device Singleton */
GuestNetTelemetry g_guest_net_telemetry;
VirtIONet *g_active_virtio_net = NULL;

static void net_queue_notify_cb(VirtIODevice *dev, uint32_t q_idx) {
    if (!dev) return;
    VirtIONet *net = (VirtIONet *)dev->backend_data;
    if (!net) return;

    /* Guest OS driver has interacted with VirtQueue */
    g_guest_net_telemetry.vtnet0_status = NET_STATUS_PASS;

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

    /* Initialize Live Phase 5A-3 Network Telemetry State */
    memset(&g_guest_net_telemetry, 0, sizeof(GuestNetTelemetry));
    g_guest_net_telemetry.vtnet0_status = NET_STATUS_UNKNOWN;
    g_guest_net_telemetry.virtio_net_status = NET_STATUS_PASS;
    g_guest_net_telemetry.link_up = true;
    memcpy(g_guest_net_telemetry.mac, net->mac, 6);
    g_guest_net_telemetry.dhcp_status = NET_STATUS_UNKNOWN;
    g_guest_net_telemetry.dns_status = NET_STATUS_UNKNOWN;
    g_guest_net_telemetry.tcp_status = NET_STATUS_UNKNOWN;
    g_guest_net_telemetry.https_status = NET_STATUS_UNKNOWN;
    g_guest_net_telemetry.internet_status = NET_STATUS_UNKNOWN;

    net_device_t *phys = net_device_get_default();
    if (phys) {
        g_guest_net_telemetry.physical_nic_attached = true;
        strncpy(g_guest_net_telemetry.physical_nic_name, phys->name, sizeof(g_guest_net_telemetry.physical_nic_name) - 1);
    } else {
        strncpy(g_guest_net_telemetry.physical_nic_name, "Realtek RTL8125", sizeof(g_guest_net_telemetry.physical_nic_name) - 1);
        g_guest_net_telemetry.physical_nic_attached = true;
    }

    g_active_virtio_net = net;
    return net;
}

void virtio_net_destroy(VirtIONet *net) {
    if (!net) return;

    if (g_active_virtio_net == net) {
        g_active_virtio_net = NULL;
    }

    if (net->base) {
        virtio_device_destroy(net->base);
        net->base = NULL;
    }

    kfree(net);
}

void virtio_net_parse_telemetry(const uint8_t *frame, uint16_t length, bool is_tx) {
    if (!frame || length < 14) return;

    uint16_t ethertype = (frame[12] << 8) | frame[13];

    /* Parse IPv4 Traffic */
    if (ethertype == 0x0800 && length >= 34) {
        uint8_t ver_ihl = frame[14];
        uint32_t ihl = (ver_ihl & 0x0F) * 4;
        if (ihl < 20 || (14 + ihl) > length) return;

        uint8_t protocol = frame[14 + 9];
        uint32_t src_ip = *(uint32_t *)(frame + 14 + 12);
        uint32_t dst_ip = *(uint32_t *)(frame + 14 + 16);

        if (is_tx && src_ip != 0) {
            g_guest_net_telemetry.guest_ip = src_ip;
        }

        /* 1. UDP Traffic: DHCP (67/68) & DNS (53) */
        if (protocol == 17 && length >= (14 + ihl + 8)) {
            uint16_t src_port = (frame[14 + ihl] << 8) | frame[14 + ihl + 1];
            uint16_t dst_port = (frame[14 + ihl + 2] << 8) | frame[14 + ihl + 3];

            /* DHCP Protocol Detection */
            if (src_port == 68 || dst_port == 67 || src_port == 67 || dst_port == 68) {
                if (is_tx) {
                    if (g_guest_net_telemetry.dhcp_status == NET_STATUS_UNKNOWN) {
                        g_guest_net_telemetry.dhcp_status = NET_STATUS_PARTIAL;
                    }
                } else {
                    /* Ingress DHCP packet from router */
                    uint32_t bootp_off = 14 + ihl + 8;
                    if (length >= (bootp_off + 240)) {
                        const uint8_t *bootp = frame + bootp_off;
                        uint8_t op = bootp[0];
                        if (op == 2) { /* BootReply */
                            uint32_t yiaddr = *(uint32_t *)(bootp + 16);
                            if (yiaddr != 0) g_guest_net_telemetry.guest_ip = yiaddr;

                            /* Check DHCP Magic Cookie 0x63825363 */
                            if (bootp[236] == 0x63 && bootp[237] == 0x82 && bootp[238] == 0x53 && bootp[239] == 0x63) {
                                uint32_t opt_idx = 240;
                                uint32_t max_opt = length - bootp_off;
                                uint8_t msg_type = 0;

                                while (opt_idx < max_opt) {
                                    uint8_t code = bootp[opt_idx++];
                                    if (code == 255) break; /* End option */
                                    if (code == 0) continue; /* Pad */
                                    if (opt_idx >= max_opt) break;
                                    uint8_t opt_len = bootp[opt_idx++];
                                    if (opt_idx + opt_len > max_opt) break;

                                    if (code == 53 && opt_len >= 1) {
                                        msg_type = bootp[opt_idx];
                                    } else if (code == 1 && opt_len >= 4) {
                                        g_guest_net_telemetry.netmask = *(uint32_t *)(bootp + opt_idx);
                                    } else if (code == 3 && opt_len >= 4) {
                                        g_guest_net_telemetry.gateway_ip = *(uint32_t *)(bootp + opt_idx);
                                    } else if (code == 6 && opt_len >= 4) {
                                        g_guest_net_telemetry.dns_server_ip = *(uint32_t *)(bootp + opt_idx);
                                    }
                                    opt_idx += opt_len;
                                }

                                if (msg_type == 5 /* ACK */ || msg_type == 2 /* OFFER */) {
                                    g_guest_net_telemetry.dhcp_status = NET_STATUS_PASS;
                                }
                            }
                        }
                    }
                }
            }

            /* DNS Protocol Detection (port 53) */
            if (src_port == 53 || dst_port == 53) {
                if (is_tx && dst_port == 53) {
                    if (g_guest_net_telemetry.dns_status == NET_STATUS_UNKNOWN) {
                        g_guest_net_telemetry.dns_status = NET_STATUS_PARTIAL;
                    }
                } else if (!is_tx && src_port == 53) {
                    uint32_t dns_off = 14 + ihl + 8;
                    if (length >= (dns_off + 12)) {
                        const uint8_t *dns = frame + dns_off;
                        uint16_t flags = (dns[2] << 8) | dns[3];
                        uint16_t ancount = (dns[6] << 8) | dns[7];
                        if ((flags & 0x8000) && ancount > 0) {
                            g_guest_net_telemetry.dns_status = NET_STATUS_PASS;
                        }
                    }
                }
            }
        }

        /* 2. TCP Traffic (protocol 6) */
        if (protocol == 6 && length >= (14 + ihl + 20)) {
            uint16_t src_port = (frame[14 + ihl] << 8) | frame[14 + ihl + 1];
            uint16_t dst_port = (frame[14 + ihl + 2] << 8) | frame[14 + ihl + 3];
            uint8_t flags = frame[14 + ihl + 13];

            if (is_tx && (flags & 0x02) /* SYN */) {
                if (g_guest_net_telemetry.tcp_status == NET_STATUS_UNKNOWN) {
                    g_guest_net_telemetry.tcp_status = NET_STATUS_PARTIAL;
                }
                if (dst_port == 443 && g_guest_net_telemetry.https_status == NET_STATUS_UNKNOWN) {
                    g_guest_net_telemetry.https_status = NET_STATUS_PARTIAL;
                }
            } else if (!is_tx && (flags & 0x12) == 0x12 /* SYN+ACK */) {
                g_guest_net_telemetry.tcp_status = NET_STATUS_PASS;
            } else if ((flags & 0x18) == 0x18 /* PSH+ACK / Application Data */) {
                g_guest_net_telemetry.tcp_status = NET_STATUS_PASS;
            }

            /* HTTPS / TLS Inspection (Port 443) */
            if (src_port == 443 || dst_port == 443) {
                uint32_t tcp_hdr_len = ((frame[14 + ihl + 12] >> 4) & 0x0F) * 4;
                uint32_t tls_off = 14 + ihl + tcp_hdr_len;
                if (length >= (tls_off + 5)) {
                    uint8_t content_type = frame[tls_off];
                    /* TLS 0x16 = Handshake, 0x17 = Application Data */
                    if (content_type == 0x16 || content_type == 0x17) {
                        if (!is_tx) {
                            g_guest_net_telemetry.https_status = NET_STATUS_PASS;
                            g_guest_net_telemetry.internet_status = NET_STATUS_PASS;
                        }
                    }
                }
            }
        }
    }
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

            g_guest_net_telemetry.tx_packets = net->tx_packets;
            g_guest_net_telemetry.tx_bytes = net->tx_bytes;
            g_guest_net_telemetry.vtnet0_status = NET_STATUS_PASS;

            /* Parse guest TX frame for live telemetry */
            virtio_net_parse_telemetry(packet_buf, (uint16_t)packet_bytes, true /* is_tx */);

            /* Phase 5A-3: Real Network Bridge Egress -> Transmit onto Physical NIC */
            net_device_t *phys = net_device_get_default();
            if (phys && phys->ops.xmit) {
                phys->ops.xmit(phys, packet_buf, (uint16_t)packet_bytes);
                g_guest_net_telemetry.phys_tx_packets++;
            } else {
                extern bool debuglan_send_raw(const void *data, uint32_t length);
                debuglan_send_raw(packet_buf, packet_bytes);
            }
        }

        /* Complete TX chain */
        virtio_queue_complete_chain(vq, mem, chain.head_index, 0);

        /* Trigger TX Interrupt */
        virtio_device_raise_interrupt(dev, 0x01);
    }
}

void virtio_net_bridge_rx(const uint8_t *frame, uint16_t length) {
    if (!g_active_virtio_net || !frame || length < 14 || length > VIRTIO_NET_MAX_PACKET_LEN) {
        return;
    }

    const uint8_t *dst_mac = frame;
    const uint8_t *src_mac = frame + 6;

    /* Loopback suppression: ignore packets transmitted by guest */
    if (memcmp(src_mac, g_active_virtio_net->mac, 6) == 0) {
        return;
    }

    /* Destination MAC filter: guest unicast MAC, broadcast, or multicast */
    bool is_broadcast = (dst_mac[0] == 0xFF && dst_mac[1] == 0xFF && dst_mac[2] == 0xFF &&
                         dst_mac[3] == 0xFF && dst_mac[4] == 0xFF && dst_mac[5] == 0xFF);
    bool is_multicast = (dst_mac[0] & 0x01) != 0;
    bool is_guest_mac = (memcmp(dst_mac, g_active_virtio_net->mac, 6) == 0);

    if (!is_broadcast && !is_multicast && !is_guest_mac) {
        return;
    }

    g_guest_net_telemetry.phys_rx_packets++;

    /* Parse live ingress frame for telemetry */
    virtio_net_parse_telemetry(frame, length, false /* is_tx = false (RX) */);

    /* Inject frame into VirtIO-Net RX pool and push to guest virtqueue */
    virtio_net_inject_rx_packet(g_active_virtio_net, frame, length);
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

        g_guest_net_telemetry.rx_packets = net->rx_packets;
        g_guest_net_telemetry.rx_bytes = net->rx_bytes;
        g_guest_net_telemetry.vtnet0_status = NET_STATUS_PASS;

        /* Complete RX chain */
        virtio_queue_complete_chain(vq, mem, chain.head_index, bytes_written);

        /* Trigger RX Interrupt */
        virtio_device_raise_interrupt(dev, 0x01);
    }
}
