#include "usb_forensic_trace.h"
#include <stddef.h>

USBForensicEngine g_usb_forensic = {0};

static const char* stage_names[USB_STAGE_COUNT] = {
    "NONE",
    "XHCI_CONTROLLER_STARTED",
    "ENABLE_SLOT_CMD_SENT",
    "ENABLE_SLOT_EVENT_RECEIVED",
    "ADDRESS_DEVICE_CMD_SENT",
    "ADDRESS_DEVICE_EVENT_RECEIVED",
    "DEVICE_DESCRIPTOR_RECEIVED",
    "CONFIG_DESCRIPTOR_RECEIVED",
    "SET_CONFIGURATION_SENT",
    "SET_CONFIGURATION_COMPLETED",
    "CONFIGURE_ENDPOINT_CMD_SENT",
    "CONFIGURE_ENDPOINT_EVENT_RECEIVED",
    "INTERRUPT_IN_RING_CREATED",
    "INTERRUPT_IN_TRB_QUEUED",
    "INTERRUPT_IN_DOORBELL_RUNG",
    "FIRST_TRANSFER_EVENT_RECEIVED",
    "HID_BIND_COMPLETED",
    "HIDA_ROUTER_ACTIVE",
    "FIRST_MOUSE_PACKET",
    "FIRST_KEYBOARD_PACKET"
};

extern volatile uint64_t g_irq0_ticks;

void usb_forensic_init(void) {
    g_usb_forensic.last_successful_stage = 0;
    g_usb_forensic.first_failed_stage = 0;
    g_usb_forensic.is_frozen = false;
    g_usb_forensic.start_tick = g_irq0_ticks;
    g_usb_forensic.failure_tick = 0;

    for (int i = 0; i < USB_STAGE_COUNT; i++) {
        g_usb_forensic.stages[i].name = stage_names[i];
        g_usb_forensic.stages[i].status = STAGE_STATUS_WAIT;
        g_usb_forensic.stages[i].pass_tick = 0;
    }

    g_usb_forensic.suspect_subsystem = "SYSTEM INITIALIZING";
    g_usb_forensic.source_file = "kernel/drivers/usb/core/usb_enum.c";
    g_usb_forensic.expected_next_event = "XHCI_CONTROLLER_STARTED";
    g_usb_forensic.confidence_pct = 100;
    g_usb_forensic.root_cause_desc = "Pipeline Starting";
}

void usb_forensic_mark_stage(USBForensicStage stage, bool success) {
    if (stage <= 0 || stage >= USB_STAGE_COUNT) return;
    if (g_usb_forensic.is_frozen) return;

    extern void debuglan_log_subsys(const char* subsys, const char* fmt, ...);
    if (stage_names[stage]) {
        debuglan_log_subsys("USB", "%s", stage_names[stage]);
    }

    if (success) {
        g_usb_forensic.stages[stage].status = STAGE_STATUS_OK;
        g_usb_forensic.stages[stage].pass_tick = g_irq0_ticks;
        if (stage > g_usb_forensic.last_successful_stage) {
            g_usb_forensic.last_successful_stage = stage;
        }
    } else {
        g_usb_forensic.stages[stage].status = STAGE_STATUS_FAIL;
        if (g_usb_forensic.first_failed_stage == 0) {
            g_usb_forensic.first_failed_stage = stage;
            g_usb_forensic.is_frozen = true;
            g_usb_forensic.failure_tick = g_irq0_ticks;
        }
    }
}

static void run_root_cause_estimator(uint32_t failed_stage) {
    switch (failed_stage) {
        case USB_STAGE_ENABLE_SLOT_EVENT_RECEIVED:
            g_usb_forensic.suspect_subsystem = "XHCI COMMAND RING";
            g_usb_forensic.source_file = "kernel/drivers/usb/host/xhci/xhci_cmd.c";
            g_usb_forensic.expected_next_event = "TRB_COMMAND_COMPLETION_EVENT (Enable Slot)";
            g_usb_forensic.confidence_pct = 95;
            g_usb_forensic.root_cause_desc = "95% Enable Slot Command Timeout";
            break;

        case USB_STAGE_ADDRESS_DEVICE_EVENT_RECEIVED:
            g_usb_forensic.suspect_subsystem = "XHCI COMMAND RING";
            g_usb_forensic.source_file = "kernel/drivers/usb/host/xhci/xhci_cmd.c";
            g_usb_forensic.expected_next_event = "TRB_COMMAND_COMPLETION_EVENT (Address Device)";
            g_usb_forensic.confidence_pct = 92;
            g_usb_forensic.root_cause_desc = "92% Address Device Command Failure";
            break;

        case USB_STAGE_DEVICE_DESCRIPTOR_RECEIVED:
        case USB_STAGE_CONFIG_DESCRIPTOR_RECEIVED:
            g_usb_forensic.suspect_subsystem = "XHCI TRANSFER RING (EP0)";
            g_usb_forensic.source_file = "kernel/drivers/usb/host/xhci/xhci_transfer.c";
            g_usb_forensic.expected_next_event = "TRANSFER_EVENT (Control GET_DESCRIPTOR)";
            g_usb_forensic.confidence_pct = 88;
            g_usb_forensic.root_cause_desc = "88% EP0 Control Transfer Timeout";
            break;

        case USB_STAGE_SET_CONFIGURATION_COMPLETED:
            g_usb_forensic.suspect_subsystem = "USB ENUMERATION";
            g_usb_forensic.source_file = "kernel/drivers/usb/core/usb_enum.c";
            g_usb_forensic.expected_next_event = "TRANSFER_EVENT (Control SET_CONFIGURATION)";
            g_usb_forensic.confidence_pct = 90;
            g_usb_forensic.root_cause_desc = "90% SET_CONFIGURATION Request Failure";
            break;

        case USB_STAGE_CONFIGURE_ENDPOINT_EVENT_RECEIVED:
            g_usb_forensic.suspect_subsystem = "XHCI COMMAND RING";
            g_usb_forensic.source_file = "kernel/drivers/usb/host/xhci/xhci_transfer.c";
            g_usb_forensic.expected_next_event = "TRB_COMMAND_COMPLETION_EVENT (Configure EP)";
            g_usb_forensic.confidence_pct = 92;
            g_usb_forensic.root_cause_desc = "92% Configure Endpoint Command Failure";
            break;

        case USB_STAGE_FIRST_TRANSFER_EVENT_RECEIVED:
            g_usb_forensic.suspect_subsystem = "XHCI EVENT RING";
            g_usb_forensic.source_file = "kernel/drivers/usb/host/xhci/xhci_transfer.c";
            g_usb_forensic.expected_next_event = "TRANSFER_EVENT (Interrupt IN EP1)";
            g_usb_forensic.confidence_pct = 89;
            g_usb_forensic.root_cause_desc = "89% Interrupt IN Transfer Timeout / Avg TRB Len";
            break;

        case USB_STAGE_HID_BIND_COMPLETED:
            g_usb_forensic.suspect_subsystem = "USB HID DRIVER";
            g_usb_forensic.source_file = "kernel/drivers/usb/class/usb_hid.c";
            g_usb_forensic.expected_next_event = "HID_BIND_COMPLETED";
            g_usb_forensic.confidence_pct = 94;
            g_usb_forensic.root_cause_desc = "94% HID Class Driver Binding Failure";
            break;

        case USB_STAGE_HIDA_ROUTER_ACTIVE:
            g_usb_forensic.suspect_subsystem = "HIDA INPUT ROUTER";
            g_usb_forensic.source_file = "kernel/drivers/input/core/hida.c";
            g_usb_forensic.expected_next_event = "HIDA_ROUTER_ACTIVE";
            g_usb_forensic.confidence_pct = 91;
            g_usb_forensic.root_cause_desc = "91% HIDA Ownership Filter Dropping Events";
            break;

        case USB_STAGE_FIRST_MOUSE_PACKET:
        case USB_STAGE_FIRST_KEYBOARD_PACKET:
            g_usb_forensic.suspect_subsystem = "USB HID DRIVER";
            g_usb_forensic.source_file = "kernel/drivers/usb/class/usb_hid.c";
            g_usb_forensic.expected_next_event = "HID_REPORT_RECEIVED";
            g_usb_forensic.confidence_pct = 85;
            g_usb_forensic.root_cause_desc = "85% HID Report Packet Decoder Idle";
            break;

        default:
            g_usb_forensic.suspect_subsystem = "XHCI HOST CONTROLLER";
            g_usb_forensic.source_file = "kernel/drivers/usb/host/xhci/xhci.c";
            g_usb_forensic.expected_next_event = "XHCI_HARDWARE_EVENT";
            g_usb_forensic.confidence_pct = 80;
            g_usb_forensic.root_cause_desc = "80% General xHCI Hardware Stall";
            break;
    }
}

void usb_forensic_update_tick(void) {
    if (g_usb_forensic.is_frozen) return;

    uint32_t last = g_usb_forensic.last_successful_stage;
    if (last == 0) return; // Not started yet

    if (last >= 19) {
        // Pipeline complete!
        g_usb_forensic.suspect_subsystem = "NONE (ALL PASS)";
        g_usb_forensic.source_file = "NONE";
        g_usb_forensic.expected_next_event = "USB_INPUT_RUNTIME";
        g_usb_forensic.confidence_pct = 100;
        g_usb_forensic.root_cause_desc = "100% Pipeline Healthy";
        return;
    }

    uint64_t last_pass_tick = g_usb_forensic.stages[last].pass_tick;
    uint64_t current_tick = g_irq0_ticks;

    // Timeout threshold: 3000ms (3 seconds)
    if (last_pass_tick > 0 && (current_tick - last_pass_tick > 3000)) {
        uint32_t failed = last + 1;
        g_usb_forensic.first_failed_stage = failed;
        g_usb_forensic.stages[failed].status = STAGE_STATUS_STALL;
        g_usb_forensic.is_frozen = true;
        g_usb_forensic.failure_tick = current_tick;
        run_root_cause_estimator(failed);
    }
}
