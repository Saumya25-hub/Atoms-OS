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

// USBCMD Register Bits
#define XHCI_CMD_RS    (1 << 0)
#define XHCI_CMD_HCRST (1 << 1)
#define XHCI_CMD_INTE  (1 << 2)
#define XHCI_CMD_HSEE  (1 << 3)

// USBSTS Register Bits
#define XHCI_STS_HCH   (1 << 0)
#define XHCI_STS_HSE   (1 << 2)
#define XHCI_STS_EINT  (1 << 3)
#define XHCI_STS_PCD   (1 << 4)

// Interrupter Register Bits
#define XHCI_IMAN_IP   (1 << 0)
#define XHCI_IMAN_IE   (1 << 1)
#define XHCI_ERDP_EHB  (1 << 3)

// Completion Codes
#define CC_SUCCESS               1
#define CC_TRB_ERROR             5
#define CC_STALL_ERROR           6
#define CC_BABBLE_DETECTED_ERROR 7
#define CC_SHORT_PACKET          13

// Phase 3 Hardware Interrupt Delivery Mode
typedef enum {
    XHCI_INTR_MODE_NONE = 0,
    XHCI_INTR_MODE_POLLED = 1,
    XHCI_INTR_MODE_MSI = 2,
    XHCI_INTR_MODE_MSIX = 3
} xhci_intr_delivery_mode_t;

extern volatile xhci_intr_delivery_mode_t g_xhci_intr_delivery_mode;
extern volatile uint64_t g_xhci_interrupts;
extern volatile uint64_t g_bsme_stall_recoveries;

#include "kernel/core/interrupt/include/isr.h"

uint64_t xhci_isr(registers_t* regs);
bool xhci_reset_endpoint(uint8_t slot_id, uint8_t ep_index);
bool xhci_cmd_set_tr_dequeue_ptr(uint8_t slot_id, uint8_t ep_index, uint64_t dequeue_ptr, uint8_t dcs);
void xhci_process_events(void);

// Ring Helpers
void xhci_ring_init(XHCIRing* ring, uint32_t num_trbs);
void xhci_ring_enqueue(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control);

extern volatile bool g_xhci_ep0_transfer_complete[256];
extern volatile uint32_t g_xhci_ep0_completion_code[256];
extern volatile uint32_t g_xhci_ep0_transfer_length[256];

extern volatile bool g_xhci_ep_transfer_complete[256][32];
extern volatile uint32_t g_xhci_ep_completion_code[256][32];
extern volatile uint32_t g_xhci_ep_transfer_length[256][32];

struct USBDevice;
bool xhci_configure_bulk_endpoints(struct USBDevice* dev, uint8_t in_ep, uint16_t in_max_packet, uint8_t out_ep, uint16_t out_max_packet);
bool xhci_bulk_transfer(struct USBDevice* dev, uint8_t ep_addr, void* buffer, uint32_t length, uint32_t* actual_length, uint32_t timeout_ms);

#endif // SIGNATURES_XHCI_H
