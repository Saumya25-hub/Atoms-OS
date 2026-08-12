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
    uint32_t drop_context_flags;
    uint32_t add_context_flags;
    uint32_t reserved1[5];
    uint32_t config;
} __attribute__((packed)) XHCIInputControlContext;

typedef struct {
    uint32_t field1; 
    uint32_t field2; 
    uint32_t field3; 
    uint32_t field4; 
    uint32_t reserved[4];
} __attribute__((packed)) XHCISlotContext;

typedef struct {
    uint32_t field1; 
    uint32_t field2; 
    uint64_t tr_dequeue_ptr;
    uint32_t field5; 
    uint32_t reserved[3];
} __attribute__((packed)) XHCIEndpointContext;

typedef struct {
    uint8_t data[2048];
} __attribute__((packed, aligned(64))) XHCIInputContext;

typedef struct {
    uint8_t data[2048];
} __attribute__((packed, aligned(64))) XHCIDeviceContext;

extern uint32_t g_xhci_context_size;

static inline XHCIInputControlContext* xhci_get_input_ctrl_ctx(XHCIInputContext* ctx) {
    return (XHCIInputControlContext*)ctx;
}

static inline XHCISlotContext* xhci_get_slot_ctx(void* ctx, bool is_input) {
    uint32_t index = is_input ? 1 : 0;
    return (XHCISlotContext*)((uint8_t*)ctx + (index * g_xhci_context_size));
}

static inline XHCIEndpointContext* xhci_get_ep_ctx(void* ctx, bool is_input, uint8_t ep_index) {
    uint32_t index = is_input ? (ep_index + 2) : (ep_index + 1);
    return (XHCIEndpointContext*)((uint8_t*)ctx + (index * g_xhci_context_size));
}

typedef struct {
    XHCITrb* trbs;
    uint64_t phys_base;
    uint32_t size; // in TRBs
    uint32_t enqueue;
    uint32_t dequeue;
    uint8_t cycle;

} XHCIRing;

// USBLEGSUP Extended Capability Definitions
#define XHCI_EXT_CAP_LEGSUP 1
#define XHCI_EXT_CAP_PROTOCOL 2
#define XHCI_BIOS_OWNED_SEMAPHORE (1 << 16)
#define XHCI_OS_OWNED_SEMAPHORE   (1 << 24)

// Initialize xHCI Host Controller
void xhci_init(void);
void xhci_bios_handoff(uint64_t mmio_base, uint32_t hccparams1);

// DMA Allocation Helper
void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);

// Ring Helpers
void xhci_ring_init(XHCIRing* ring, uint32_t num_trbs);
void xhci_ring_enqueue(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control);

#endif // SIGNATURES_XHCI_H
