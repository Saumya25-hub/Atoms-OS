#ifndef SIGNATURES_XHCI_H
#define SIGNATURES_XHCI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define XHCI_PCI_PROGIF 0x30

// PCI Command Register Offsets & Bits
#define PCI_COMMAND_OFFSET 0x04
#define PCI_COMMAND_MEMORY (1 << 1)
#define PCI_COMMAND_MASTER (1 << 2)

// TRB Types
#define TRB_NORMAL 1
#define TRB_SETUP_STAGE 2
#define TRB_DATA_STAGE 3
#define TRB_STATUS_STAGE 4
#define TRB_LINK 6
#define TRB_EVENT_DATA 7
#define TRB_NO_OP 8
#define TRB_ENABLE_SLOT_CMD 9
#define TRB_DISABLE_SLOT_CMD 10
#define TRB_ADDRESS_DEVICE_CMD 11
#define TRB_CONFIGURE_ENDPOINT_CMD 12
#define TRB_EVALUATE_CONTEXT_CMD 13
#define TRB_RESET_ENDPOINT_CMD 14
#define TRB_STOP_ENDPOINT_CMD 15
#define TRB_SET_TR_DEQUEUE_PTR_CMD 16
#define TRB_RESET_DEVICE_CMD 17
#define TRB_TRANSFER_EVENT 32
#define TRB_COMMAND_COMPLETION_EVENT 33
#define TRB_PORT_STATUS_CHANGE_EVENT 34

typedef struct {
    uint32_t param1;
    uint32_t param2;
    uint32_t status;
    uint32_t control;
} __attribute__((packed)) XHCITrb;

typedef struct {
    uint64_t ring_segment_base_address;
    uint16_t ring_segment_size;
    uint16_t reserved1;
    uint32_t reserved2;
} __attribute__((packed)) XHCIEventRingSegmentTableEntry;

typedef struct {
    uint64_t pointers[256]; 
} __attribute__((packed, aligned(64))) XHCIDcbaa;

typedef struct {
    XHCITrb* trbs;
    uint64_t phys_base;
    uint32_t size; // in TRBs
    uint32_t enqueue;
    uint32_t dequeue;
    uint8_t cycle;
} XHCIRing;

// Initialize xHCI Host Controller
void xhci_init(void);

// DMA Allocation Helper
void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);

// Ring Helpers
void xhci_ring_init(XHCIRing* ring, uint32_t num_trbs);
void xhci_ring_enqueue(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control);

#endif // SIGNATURES_XHCI_H
