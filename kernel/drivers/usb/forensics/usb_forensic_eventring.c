#include "usb_forensic_center.h"
#include "kernel/core/lib/include/string.h"

static const char* xhci_completion_code_to_string(uint32_t code) {
    switch (code) {
        case 1:  return "Success";
        case 2:  return "Data Buffer Error";
        case 3:  return "Babble Detected Error";
        case 4:  return "USB Transaction Error";
        case 5:  return "TRB Error";
        case 6:  return "Stall Error";
        case 7:  return "Resource Error";
        case 8:  return "Bandwidth Error";
        case 9:  return "No Slots Available Error";
        case 10: return "Invalid Stream Type Error";
        case 11: return "Slot Not Enabled Error";
        case 12: return "Endpoint Not Enabled Error";
        case 13: return "Short Packet";
        case 14: return "Ring Underrun";
        case 15: return "Ring Overrun";
        case 16: return "VF Event Ring Full Error";
        case 17: return "Parameter Error";
        case 18: return "Bandwidth Overrun Error";
        case 19: return "Context State Error";
        case 20: return "No Ping Response Error";
        case 21: return "Event Ring Full Error";
        case 22: return "Incompatible Device Error";
        case 23: return "Missed Service Error";
        case 24: return "Command Ring Stopped";
        case 25: return "Command Aborted";
        case 26: return "Stopped";
        case 27: return "Stopped - Length Invalid";
        default: return "Unknown Completion Code";
    }
}

static const char* xhci_trb_type_to_string(uint32_t type) {
    switch (type) {
        case 32: return "Transfer Event";
        case 33: return "Command Completion Event";
        case 34: return "Port Status Change Event";
        case 35: return "Bandwidth Request Event";
        case 36: return "Doorbell Event";
        case 37: return "Host Controller Event";
        case 38: return "Device Notification Event";
        case 39: return "MFINDEX Wrap Event";
        default: return "Other Event";
    }
}

void usb_forensic_log_event(uint32_t type, uint32_t code, uint8_t slot, uint8_t ep, uint64_t trb_ptr, uint32_t status, uint32_t control) {
    ForensicEventRingPanel* panel = &g_forensic_center.event_ring;
    uint32_t idx = panel->head;

    ForensicEventEntry* entry = &panel->entries[idx];
    entry->event_id = ++panel->total_events;
    entry->type = type;
    strcpy(entry->type_str, xhci_trb_type_to_string(type));
    entry->completion_code = code;
    entry->completion_str = xhci_completion_code_to_string(code);
    entry->slot_id = slot;
    entry->endpoint_id = ep;
    entry->trb_ptr = trb_ptr;
    entry->raw_status = status;
    entry->raw_control = control;

    panel->head = (panel->head + 1) % FORENSIC_EVENT_RING_CAPACITY;
    if (panel->head == panel->tail) {
        panel->tail = (panel->tail + 1) % FORENSIC_EVENT_RING_CAPACITY;
    }

    strcpy(g_forensic_center.interrupts.last_event_type, entry->type_str);
}
