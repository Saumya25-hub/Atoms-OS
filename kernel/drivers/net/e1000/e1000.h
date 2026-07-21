#ifndef SIGNATURES_E1000_H
#define SIGNATURES_E1000_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/pci/pci.h"

#define E1000_NUM_TX_DESC       64
#define E1000_NUM_RX_DESC       64
#define E1000_MAX_FRAME_SIZE    1518
#define E1000_RX_QUEUE_CAPACITY 16

typedef enum {
    E1000_STATE_UNINITIALIZED = 0,
    E1000_STATE_PCI_FOUND,
    E1000_STATE_PCI_ENABLED,
    E1000_STATE_MMIO_READY,
    E1000_STATE_RESET_COMPLETE,
    E1000_STATE_MAC_READY,
    E1000_STATE_TX_READY,
    E1000_STATE_RX_READY,
    E1000_STATE_READY,
    E1000_STATE_FAILED
} E1000State;

// Legacy E1000 TX Descriptor (16 bytes)
struct e1000_tx_desc {
    uint64_t buffer_addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t special;
} __attribute__((packed));

// Legacy E1000 RX Descriptor (16 bytes)
struct e1000_rx_desc {
    uint64_t buffer_addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed));

typedef struct {
    uint8_t data[E1000_MAX_FRAME_SIZE];
    uint16_t length;
} E1000Frame;

typedef struct {
    E1000Frame frames[E1000_RX_QUEUE_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t dropped;
} E1000RxQueue;

typedef struct {
    PCIDevice* pci_device;
    
    uint64_t mmio_phys_base;
    uint64_t mmio_virt_base;
    uint64_t mmio_size;
    
    uint8_t mac_addr[6];
    const char* mac_source;
    
    // TX Ring Datapath
    struct e1000_tx_desc* tx_ring;
    uint64_t tx_ring_phys;
    uint8_t* tx_buffers[E1000_NUM_TX_DESC];
    uint64_t tx_buffers_phys[E1000_NUM_TX_DESC];
    uint32_t tx_head;
    uint32_t tx_tail;
    
    // RX Ring Datapath
    struct e1000_rx_desc* rx_ring;
    uint64_t rx_ring_phys;
    uint8_t* rx_buffers[E1000_NUM_RX_DESC];
    uint64_t rx_buffers_phys[E1000_NUM_RX_DESC];
    uint32_t rx_cur;
    
    // Bounded Kernel RX Queue
    E1000RxQueue rx_queue;
    
    E1000State state;
} E1000Device;

void e1000_init(void);
E1000Device* e1000_get_device(void);

// Datapath APIs
bool e1000_transmit_raw(const void* frame, uint16_t length);
bool e1000_poll_receive(E1000Frame* out_frame);

#endif // SIGNATURES_E1000_H
