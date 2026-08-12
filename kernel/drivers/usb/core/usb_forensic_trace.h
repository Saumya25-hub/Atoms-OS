#ifndef ATOMS_USB_FORENSIC_TRACE_H
#define ATOMS_USB_FORENSIC_TRACE_H

#include <stdint.h>
#include <stdbool.h>

// 19 Independent Forensic Pipeline Stages
typedef enum {
    USB_STAGE_NONE = 0,
    USB_STAGE_XHCI_CONTROLLER_STARTED = 1,
    USB_STAGE_ENABLE_SLOT_CMD_SENT = 2,
    USB_STAGE_ENABLE_SLOT_EVENT_RECEIVED = 3,
    USB_STAGE_ADDRESS_DEVICE_CMD_SENT = 4,
    USB_STAGE_ADDRESS_DEVICE_EVENT_RECEIVED = 5,
    USB_STAGE_DEVICE_DESCRIPTOR_RECEIVED = 6,
    USB_STAGE_CONFIG_DESCRIPTOR_RECEIVED = 7,
    USB_STAGE_SET_CONFIGURATION_SENT = 8,
    USB_STAGE_SET_CONFIGURATION_COMPLETED = 9,
    USB_STAGE_CONFIGURE_ENDPOINT_CMD_SENT = 10,
    USB_STAGE_CONFIGURE_ENDPOINT_EVENT_RECEIVED = 11,
    USB_STAGE_INTERRUPT_IN_RING_CREATED = 12,
    USB_STAGE_INTERRUPT_IN_TRB_QUEUED = 13,
    USB_STAGE_INTERRUPT_IN_DOORBELL_RUNG = 14,
    USB_STAGE_FIRST_TRANSFER_EVENT_RECEIVED = 15,
    USB_STAGE_HID_BIND_COMPLETED = 16,
    USB_STAGE_HIDA_ROUTER_ACTIVE = 17,
    USB_STAGE_FIRST_MOUSE_PACKET = 18,
    USB_STAGE_FIRST_KEYBOARD_PACKET = 19,
    USB_STAGE_COUNT = 20
} USBForensicStage;

typedef enum {
    STAGE_STATUS_WAIT = 0,
    STAGE_STATUS_OK = 1,
    STAGE_STATUS_FAIL = 2,
    STAGE_STATUS_STALL = 3
} USBStageStatus;

typedef struct {
    const char* name;
    USBStageStatus status;
    uint64_t pass_tick;
} USBStageDescriptor;

typedef struct {
    USBStageDescriptor stages[USB_STAGE_COUNT];
    uint32_t last_successful_stage;
    uint32_t first_failed_stage;
    bool is_frozen;
    uint64_t failure_tick;
    uint64_t start_tick;
    
    // Heuristic localization results
    const char* suspect_subsystem;
    const char* source_file;
    const char* expected_next_event;
    uint32_t confidence_pct;
    const char* root_cause_desc;
} USBForensicEngine;

extern USBForensicEngine g_usb_forensic;

// Public API
void usb_forensic_init(void);
void usb_forensic_mark_stage(USBForensicStage stage, bool success);
void usb_forensic_update_tick(void);

#endif // ATOMS_USB_FORENSIC_TRACE_H
