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
    NET_STATUS_NOT_REACHED,
    NET_STATUS_WAITING,
    NET_STATUS_PARTIAL,
    NET_STATUS_PASS,
    NET_STATUS_FAIL,
    NET_STATUS_TIMEOUT
} NetGateStatus;

/* Phase 5A-3 Real Network Forensic Telemetry (10 Layers) */
typedef struct {
    /* [1] PCI / Hardware Telemetry */
    NetGateStatus pci_detected;
    uint16_t      pci_vendor_id;
    uint16_t      pci_device_id;
    bool          pci_bar_mmio_valid;
    bool          pci_mmio_accessible;
    uint8_t       pci_irq;

    /* [2] RTL8125 Driver Telemetry */
    NetGateStatus driver_initialized;
    uint8_t       driver_mac[6];
    char          driver_state[32];
    bool          phy_initialized;
    bool          link_state;
    uint32_t      negotiated_speed_mbps;
    bool          duplex_full;
    bool          tx_ring_ready;
    bool          rx_ring_ready;

    /* [3] Physical Ethernet Telemetry */
    bool          phys_link_up;
    uint64_t      phys_tx_packets;
    uint64_t      phys_rx_packets;
    uint64_t      phys_tx_bytes;
    uint64_t      phys_rx_bytes;
    uint64_t      phys_last_rx_tick;
    uint64_t      phys_last_tx_tick;
    bool          packet_activity;

    /* [4] ATOMS NetInterface Telemetry */
    NetGateStatus netif_initialized;
    char          netif_bound_name[16];
    uint8_t       netif_bound_mac[6];
    bool          netif_link_synced;
    char          netif_state_str[32];
    NetGateStatus netif_ip_config_state;

    /* [5] VirtIO-Net Device Model & Guest Interface */
    NetGateStatus virtio_pci_detected;
    NetGateStatus virtio_net_status;
    bool          virtqueues_created;
    bool          rx_queue_ready;
    bool          tx_queue_ready;
    uint8_t       device_status;
    NetGateStatus vtnet0_status;
    uint8_t       mac[6];
    uint64_t      rx_packets;
    uint64_t      tx_packets;
    uint64_t      rx_bytes;
    uint64_t      tx_bytes;
    bool          link_up;

    /* [6] DHCP Forensics (5 Distinct Stages) */
    NetGateStatus dhcp_client_started;
    NetGateStatus dhcp_discover_sent;
    NetGateStatus dhcp_offer_received;
    NetGateStatus dhcp_request_sent;
    NetGateStatus dhcp_ack_received;
    uint8_t       last_dhcp_msg_type;
    uint32_t      last_dhcp_src_ip;
    uint32_t      last_dhcp_dst_ip;
    uint32_t      dhcp_tx_count;
    uint32_t      dhcp_rx_count;
    uint32_t      dhcp_timeout_retries;
    char          last_dhcp_error[32];
    NetGateStatus dhcp_status;

    /* [7] IPv4 & ARP */
    uint32_t      guest_ip;            /* Network byte order */
    uint32_t      netmask;
    uint32_t      gateway_ip;
    NetGateStatus ipv4_status;
    NetGateStatus arp_status;
    uint32_t      arp_requests_sent;
    uint32_t      arp_replies_rcvd;

    /* [8] DNS */
    uint32_t      dns_server_ip;
    NetGateStatus dns_request_sent;
    NetGateStatus dns_response_rcvd;
    uint32_t      dns_tx_count;
    uint32_t      dns_rx_count;
    NetGateStatus dns_status;

    /* [9] TCP / HTTPS */
    NetGateStatus tcp_socket_created;
    NetGateStatus tcp_syn_sent;
    NetGateStatus tcp_syn_ack_rcvd;
    NetGateStatus tcp_status;
    NetGateStatus https_status;
    NetGateStatus internet_status;

    /* [10] Master Forensic Diagnosis & Root Cause */
    char          first_failure_subsystem[48];
    char          first_failure_reason[96];
    char          next_investigation[96];
    uint32_t      last_vm_exit_reason;
    uint64_t      last_guest_rip;
    uint64_t      last_guest_gpa;

    /* Physical NIC details */
    bool          physical_nic_attached;
    char          physical_nic_name[16];
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
void virtio_net_evaluate_forensics(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_NET_H */
