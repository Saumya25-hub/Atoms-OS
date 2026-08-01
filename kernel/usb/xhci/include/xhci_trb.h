#ifndef SIGNATURES_XHCI_TRB_H
#define SIGNATURES_XHCI_TRB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Standard xHCI TRB Types
#define TRB_TYPE_NORMAL                  1
#define TRB_TYPE_SETUP_STAGE             2
#define TRB_TYPE_DATA_STAGE              3
#define TRB_TYPE_STATUS_STAGE            4
#define TRB_TYPE_ISOCH                   5
#define TRB_TYPE_LINK                    6
#define TRB_TYPE_EVENT_DATA              7
#define TRB_TYPE_NO_OP                   8
#define TRB_TYPE_ENABLE_SLOT_CMD         9
#define TRB_TYPE_DISABLE_SLOT_CMD        10
#define TRB_TYPE_ADDRESS_DEVICE_CMD      11
#define TRB_TYPE_CONFIGURE_ENDPOINT_CMD  12
#define TRB_TYPE_EVALUATE_CONTEXT_CMD    13
#define TRB_TYPE_RESET_ENDPOINT_CMD      14
#define TRB_TYPE_STOP_ENDPOINT_CMD       15
#define TRB_TYPE_SET_TR_DEQUEUE_PTR_CMD  16
#define TRB_TYPE_RESET_DEVICE_CMD        17

#define TRB_TYPE_TRANSFER_EVENT          32
#define TRB_TYPE_COMMAND_COMPLETION_EVENT 33
#define TRB_TYPE_PORT_STATUS_CHANGE_EVENT 34
#define TRB_TYPE_BANDWIDTH_REQUEST_EVENT 35
#define TRB_TYPE_DOORBELL_EVENT          36
#define TRB_TYPE_HOST_CONTROLLER_EVENT   37
#define TRB_TYPE_DEVICE_NOTIFICATION_EVENT 38
#define TRB_TYPE_MFINDEX_WRAP_EVENT      39

// TRB Control Flags
#define TRB_CTRL_CYCLE                   (1U << 0)
#define TRB_CTRL_ENT                     (1U << 1) // Evaluate Next TRB
#define TRB_CTRL_ISP                     (1U << 2) // Interrupt on Short Packet
#define TRB_CTRL_NS                      (1U << 3) // No Snoop
#define TRB_CTRL_CH                      (1U << 4) // Chain Bit
#define TRB_CTRL_IOC                     (1U << 5) // Interrupt On Completion
#define TRB_CTRL_IDT                     (1U << 6) // Immediate Data
#define TRB_CTRL_BEI                     (1U << 9) // Block Event Interrupt
#define TRB_CTRL_DIR_IN                  (1U << 16)// Data Direction (1=IN, 0=OUT)

// TRB Completion Codes
#define TRB_COMP_INVALID                 0
#define TRB_COMP_SUCCESS                 1
#define TRB_COMP_DATA_BUFFER_ERROR       2
#define TRB_COMP_BABBLE_DETECTED         3
#define TRB_COMP_TRANSACTION_ERROR       4
#define TRB_COMP_TRB_ERROR               5
#define TRB_COMP_STALL_ERROR             6
#define TRB_COMP_RESOURCE_ERROR          7
#define TRB_COMP_BANDWIDTH_ERROR         8
#define TRB_COMP_NO_SLOTS_AVAILABLE      9
#define TRB_COMP_INVALID_STREAM_TYPE    10
#define TRB_COMP_SLOT_NOT_ENABLED       11
#define TRB_COMP_ENDPOINT_NOT_ENABLED   12
#define TRB_COMP_SHORT_PACKET           13
#define TRB_COMP_STOPPED                26

// Raw 16-byte xHCI Transfer Request Block (TRB)
typedef struct {
    uint32_t parameter_lo;
    uint32_t parameter_hi;
    uint32_t status;
    uint32_t control;
} __attribute__((packed)) xhci_trb_t;

// TRB Builder Functions
static inline void xhci_trb_build_normal(xhci_trb_t* trb, uint64_t phys_buf, uint32_t len, uint32_t interrupter_target, uint32_t flags, uint8_t cycle) {
    if (!trb) return;
    trb->parameter_lo = (uint32_t)(phys_buf & 0xFFFFFFFF);
    trb->parameter_hi = (uint32_t)((phys_buf >> 32) & 0xFFFFFFFF);
    trb->status = (len & 0x1FFFF) | ((interrupter_target & 0x3FF) << 22);
    trb->control = ((TRB_TYPE_NORMAL & 0x3F) << 10) | (flags & ~1U) | (cycle & 1U);
}

static inline void xhci_trb_build_link(xhci_trb_t* trb, uint64_t ring_next_phys, uint32_t interrupter_target, bool toggle_cycle, uint8_t cycle) {
    if (!trb) return;
    trb->parameter_lo = (uint32_t)(ring_next_phys & 0xFFFFFFFF);
    trb->parameter_hi = (uint32_t)((ring_next_phys >> 32) & 0xFFFFFFFF);
    trb->status = ((interrupter_target & 0x3FF) << 22);
    uint32_t ctrl = ((TRB_TYPE_LINK & 0x3F) << 10) | (cycle & 1U);
    if (toggle_cycle) ctrl |= (1U << 1); // TC (Toggle Cycle) bit for Link TRB is bit 1
    trb->control = ctrl;
}

static inline void xhci_trb_build_event_data(xhci_trb_t* trb, uint64_t data_param, uint32_t flags, uint8_t cycle) {
    if (!trb) return;
    trb->parameter_lo = (uint32_t)(data_param & 0xFFFFFFFF);
    trb->parameter_hi = (uint32_t)((data_param >> 32) & 0xFFFFFFFF);
    trb->status = 0;
    trb->control = ((TRB_TYPE_EVENT_DATA & 0x3F) << 10) | (flags & ~1U) | (cycle & 1U);
}

static inline void xhci_trb_build_noop(xhci_trb_t* trb, uint8_t cycle) {
    if (!trb) return;
    trb->parameter_lo = 0;
    trb->parameter_hi = 0;
    trb->status = 0;
    trb->control = ((TRB_TYPE_NO_OP & 0x3F) << 10) | (cycle & 1U);
}

#endif // SIGNATURES_XHCI_TRB_H
