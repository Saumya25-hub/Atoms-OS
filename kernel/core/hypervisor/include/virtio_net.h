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

/* Phase 5A-3 Network Evidence Gate Status */
typedef enum {
    NET_STATUS_BLOCKED = 0,
    NET_STATUS_UNKNOWN,
    NET_STATUS_PARTIAL,
    NET_STATUS_PASS,
    NET_STATUS_FAIL
} NetGateStatus;

/* Phase 5A-3 Real Network Telemetry */
typedef struct {
    /* Device status */
    NetGateStatus vtnet0_status;       /* UNKNOWN until driver negotiates or queues packets */
    NetGateStatus virtio_net_status;   /* PASS when VirtIO-Net device is created & mapped */
    bool          link_up;
    uint8_t       mac[6];

    /* Network Identity (parsed live from guest packets / DHCP ACK) */
    uint32_t      guest_ip;            /* Network byte order */
    uint32_t      netmask;
    uint32_t      gateway_ip;
    uint32_t      dns_server_ip;

    /* Live Dynamic Traffic Counters */
    uint64_t      rx_packets;
    uint64_t      tx_packets;
    uint64_t      rx_bytes;
    uint64_t      tx_bytes;

    /* Protocol Evidence Gates */
    NetGateStatus dhcp_status;         /* UNKNOWN -> PARTIAL -> PASS */
    NetGateStatus dns_status;          /* UNKNOWN -> PARTIAL -> PASS */
    NetGateStatus tcp_status;          /* UNKNOWN -> PARTIAL -> PASS */
    NetGateStatus https_status;        /* UNKNOWN -> PARTIAL -> PASS */
    NetGateStatus internet_status;     /* UNKNOWN -> PASS */

    /* Physical NIC details */
    bool          physical_nic_attached;
    char          physical_nic_name[16];
    uint64_t      phys_tx_packets;
    uint64_t      phys_rx_packets;
} GuestNetTelemetry;

extern GuestNetTelemetry g_guest_net_telemetry;
extern VirtIONet *g_active_virtio_net;

/* Core APIs */
VirtIONet *virtio_net_create(const uint8_t mac[6]);
void virtio_net_destroy(VirtIONet *net);

/* Packet Ingestion & Transmission */
void virtio_net_process_tx(VirtIONet *net);
bool virtio_net_inject_rx_packet(VirtIONet *net, const uint8_t *packet, uint32_t len);
void virtio_net_flush_rx(VirtIONet *net);

/* Phase 5A-3 Bridge & Telemetry Parser */
void virtio_net_bridge_rx(const uint8_t *frame, uint16_t length);
void virtio_net_parse_telemetry(const uint8_t *frame, uint16_t length, bool is_tx);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_NET_H */
