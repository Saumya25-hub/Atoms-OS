/* kernel/net/net_framework.h - ATOMS OS NETLIB Core Framework Header */
#ifndef SIGNATURES_NET_FRAMEWORK_H
#define SIGNATURES_NET_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NET_MAX_PACKET_SIZE 1536
#define NET_PACKET_POOL_SIZE 512
#define NET_RING_SIZE       256
#define NET_MAC_ADDR_LEN    6

/* Hardware Capability Bitmask Matrix */
typedef enum {
    NET_CAP_NONE            = 0,
    NET_CAP_CSUM_IPV4_TX    = (1 << 0),
    NET_CAP_CSUM_IPV4_RX    = (1 << 1),
    NET_CAP_CSUM_UDP_TX     = (1 << 2),
    NET_CAP_CSUM_UDP_RX     = (1 << 3),
    NET_CAP_CSUM_TCP_TX     = (1 << 4),
    NET_CAP_CSUM_TCP_RX     = (1 << 5),
    NET_CAP_64BIT_DMA       = (1 << 6),
    NET_CAP_DESC_32BYTE     = (1 << 7),  /* RTL8125 2.5G Extended Descriptor Mode */
    NET_CAP_MSIX            = (1 << 8),
    NET_CAP_VLAN_OFFLOAD    = (1 << 9)
} net_capability_t;

/* Unified Zero-Copy Lockless Packet Descriptor */
typedef struct net_packet {
    uint8_t  data[NET_MAX_PACKET_SIZE] __attribute__((aligned(64)));
    uint64_t phys_addr;
    uint16_t length;
    uint16_t offset;
    uint32_t flags;
    volatile bool in_use;
    struct net_packet* next;
} net_packet_t;

/* Device Queue Performance Statistics */
typedef struct {
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t rx_errors;
    uint64_t rx_dropped;
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t tx_errors;
    uint64_t tx_dropped;
    uint64_t dma_stalls;
} net_statistics_t;

struct net_device;

/* Network Driver Virtual Operations Table */
typedef struct net_driver_ops {
    bool (*init)(struct net_device* dev);
    bool (*open)(struct net_device* dev);
    bool (*stop)(struct net_device* dev);
    bool (*xmit)(struct net_device* dev, const void* frame, uint16_t length);
    bool (*poll_rx)(struct net_device* dev);
    void (*reclaim_tx)(struct net_device* dev);
    void (*set_mac)(struct net_device* dev, const uint8_t* mac);
} net_driver_ops_t;

#include "kernel/core/pci/pci.h"

/* Core Network Interface Abstraction Instance */
typedef struct net_device {
    char name[16];
    uint8_t mac_addr[NET_MAC_ADDR_LEN];
    
    /* Hardware Bus Parameters */
    PCIDevice* pci_dev;
    uint8_t  pci_bus;
    uint8_t  pci_slot;
    uint8_t  pci_func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  revision_id;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_device_id;
    
    uint64_t mmio_base;
    uint16_t io_base;
    bool     is_mmio;
    uint8_t  irq_line;

    /* IP Stack Parameters */
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    bool     link_up;

    /* Hardware Drivers & Stats */
    net_capability_t capabilities;
    net_driver_ops_t ops;
    net_statistics_t stats;

    void* driver_private;
} net_device_t;

/* Core NETLIB API Prototypes */
void net_framework_init(void);
net_packet_t* net_packet_allocate(void);
void net_packet_free(net_packet_t* pkt);

bool net_device_register(net_device_t* dev);
net_device_t* net_device_get_default(void);

#endif /* SIGNATURES_NET_FRAMEWORK_H */
