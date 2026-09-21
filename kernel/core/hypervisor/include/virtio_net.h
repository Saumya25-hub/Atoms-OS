/*
 * ATOMS OS — VirtIO Network Device Header
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_NET_H
#define ATOMS_VIRTIO_NET_H

#include "virtio_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIRTIO_NET_QUEUE_RX                 0
#define VIRTIO_NET_QUEUE_TX                 1
#define VIRTIO_NET_MAX_PACKET_LEN           1514
#define VIRTIO_NET_PACKET_QUEUE_DEPTH       64

/* Buffered Packet Structure */
typedef struct {
    uint8_t data[VIRTIO_NET_MAX_PACKET_LEN];
    uint32_t len;
    bool valid;
} VirtIONetPacket;

/* VirtIO Network Device Instance */
typedef struct virtio_net_dev {
    VirtIODevice *base;

    uint8_t mac[6];
    uint16_t link_status;

    /* Isolated Internal Loopback / Packet Buffer */
    VirtIONetPacket rx_pool[VIRTIO_NET_PACKET_QUEUE_DEPTH];
    uint32_t rx_head;
    uint32_t rx_tail;
    uint32_t rx_count;

    /* Metrics & Statistics */
    uint64_t tx_packets;
    uint64_t rx_packets;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint64_t dropped_packets;
} VirtIONet;

/* Core APIs */
VirtIONet *virtio_net_create(const uint8_t mac[6]);
void virtio_net_destroy(VirtIONet *net);

/* Packet Ingestion & Transmission */
void virtio_net_process_tx(VirtIONet *net);
bool virtio_net_inject_rx_packet(VirtIONet *net, const uint8_t *packet, uint32_t len);
void virtio_net_flush_rx(VirtIONet *net);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_NET_H */
