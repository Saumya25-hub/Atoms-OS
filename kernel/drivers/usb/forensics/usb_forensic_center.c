#include "usb_forensic_center.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

USBForensicCommandCenter g_forensic_center = {0};

void usb_forensic_center_init(void) {
    memset(&g_forensic_center, 0, sizeof(USBForensicCommandCenter));
    g_forensic_center.initialized = true;

    // Set initial default panel values
    strcpy(g_forensic_center.dashboard.controller_type, "xHCI");
    strcpy(g_forensic_center.dashboard.state, "INITIALIZING");
    
    strcpy(g_forensic_center.comparison.controller_used, "xHCI");
    strcpy(g_forensic_center.comparison.fallback_mode, "NONE (PRIMARY)");
    strcpy(g_forensic_center.comparison.failure_reason, "NONE");

    strcpy(g_forensic_center.hardware.motherboard, "Haswell H81 Motherboard");
    strcpy(g_forensic_center.hardware.cpu, "Intel Core i3 Haswell");
    strcpy(g_forensic_center.hardware.vendor_name, "Intel Corporation");
    g_forensic_center.hardware.vendor_id = 0x8086;

    // Initialize 19 Enumeration Stage Names
    const char* default_stages[19] = {
        "PORT_CONNECTED", "CONTROLLER_RESET", "ENABLE_SLOT_SENT", "ENABLE_SLOT_EVENT",
        "ADDRESS_DEV_SENT", "ADDRESS_DEV_EVENT", "DEV_DESC_8B_RCVD", "EVALUATE_CTX_SENT",
        "DEV_DESC_18B_RCVD", "CONFIG_HDR_9B_RCVD", "CONFIG_FULL_RCVD", "SET_CONFIG_SENT",
        "SET_CONFIG_EVENT", "EP_CONFIG_SENT", "EP_CONFIG_EVENT", "INTERRUPT_TRB_QUEUED",
        "INTERRUPT_EVENT_RCVD", "HID_REPORT_PARSED", "CURSOR_ENGINE_BOUND"
    };

    for (int i = 0; i < 19; i++) {
        strcpy(g_forensic_center.timeline.stage_names[i], default_stages[i]);
        g_forensic_center.timeline.stage_passed[i] = false;
    }

    display_print("[USB FORENSIC CENTER V1.0] Master Diagnostic Command Center Online!\n");
}

void usb_forensic_center_update(void) {
    if (!g_forensic_center.initialized) return;
    g_forensic_center.interrupts.irq_count++;
}

void usb_forensic_center_render(void) {
    if (!g_forensic_center.initialized) return;

    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);
    extern void display_print_hex(uint64_t);

    display_print("\n========================================\n");
    display_print("   USB FORENSIC COMMAND CENTER V1.0\n");
    display_print("========================================\n");

    // Panel 1 — Host Controller Dashboard
    display_print("--- PANEL 1: HOST CONTROLLER DASHBOARD ---\n");
    display_print("Controller : "); display_print(g_forensic_center.dashboard.controller_type); display_print("\n");
    display_print("PCI BDF    : 00:"); display_print_hex(g_forensic_center.dashboard.slot); display_print(".0\n");
    display_print("USBCMD     : "); display_print_hex(g_forensic_center.dashboard.usbcmd); display_print("\n");
    display_print("USBSTS     : "); display_print_hex(g_forensic_center.dashboard.usbsts); display_print("\n");
    display_print("DNCTRL     : "); display_print_hex(g_forensic_center.dashboard.dnctrl); display_print("\n");
    display_print("CONFIG     : "); display_print_hex(g_forensic_center.dashboard.config); display_print("\n");
    display_print("IRQ Count  : "); display_print_dec(g_forensic_center.dashboard.irq_count); display_print("\n");
    display_print("State      : "); display_print(g_forensic_center.dashboard.state); display_print("\n");

    // Panel 2 — Port Forensics
    display_print("--- PANEL 2: PORT FORENSICS ---\n");
    display_print("Port Count : "); display_print_dec(g_forensic_center.ports.port_count); display_print("\n");
    for (uint32_t i = 0; i < g_forensic_center.ports.port_count && i < 4; i++) {
        ForensicPortStatus* p = &g_forensic_center.ports.ports[i];
        display_print("Port "); display_print_dec(p->port_num);
        display_print(": CCS="); display_print_dec(p->ccs);
        display_print(" PED="); display_print_dec(p->ped);
        display_print(" PR="); display_print_dec(p->pr);
        display_print(" Speed="); display_print(p->speed);
        display_print(" Slot="); display_print_dec(p->slot_id);
        display_print(" Status="); display_print(p->status); display_print("\n");
    }

    // Panel 3 — Event Ring Analyzer
    display_print("--- PANEL 3: EVENT RING ANALYZER (History: ");
    display_print_dec(g_forensic_center.event_ring.total_events); display_print(" events) ---\n");

    // Panel 4 — Control Transfer Analyzer
    display_print("--- PANEL 4: CONTROL TRANSFER ANALYZER ---\n");
    display_print("bmRequestType : "); display_print_hex(g_forensic_center.control_transfer.bmRequestType); display_print("\n");
    display_print("bRequest      : "); display_print_hex(g_forensic_center.control_transfer.bRequest); display_print("\n");
    display_print("wValue        : "); display_print_hex(g_forensic_center.control_transfer.wValue); display_print("\n");
    display_print("wIndex        : "); display_print_hex(g_forensic_center.control_transfer.wIndex); display_print("\n");
    display_print("wLength       : "); display_print_dec(g_forensic_center.control_transfer.wLength); display_print("\n");
    display_print("Direction     : "); display_print(g_forensic_center.control_transfer.direction); display_print("\n");

    // Panel 5 — DMA Analyzer
    display_print("--- PANEL 5: DMA ANALYZER ---\n");
    display_print("DMA PHYS : "); display_print_hex(g_forensic_center.dma.dma_phys); display_print("\n");
    display_print("DMA VIRT : "); display_print_hex(g_forensic_center.dma.dma_virt); display_print("\n");
    display_print("Length   : "); display_print_dec(g_forensic_center.dma.length); display_print("\n");

    // Panel 6 — Interrupt Analyzer
    display_print("--- PANEL 6: INTERRUPT ANALYZER ---\n");
    display_print("IRQ Count     : "); display_print_dec(g_forensic_center.interrupts.irq_count); display_print("\n");
    display_print("Last Event    : "); display_print(g_forensic_center.interrupts.last_event_type); display_print("\n");

    // Panel 7 — Enumeration Timeline
    display_print("--- PANEL 7: ENUMERATION TIMELINE ---\n");
    display_print("Current Stage : "); display_print_dec(g_forensic_center.timeline.current_stage); display_print("\n");

    // Panel 8 — USB Device Tree
    display_print("--- PANEL 8: USB DEVICE TREE ---\n");
    display_print("Device Count  : "); display_print_dec(g_forensic_center.tree.device_count); display_print("\n");

    // Panel 9 — Controller Comparison Layer
    display_print("--- PANEL 9: CONTROLLER COMPARISON LAYER ---\n");
    display_print("Used     : "); display_print(g_forensic_center.comparison.controller_used); display_print("\n");
    display_print("Fallback : "); display_print(g_forensic_center.comparison.fallback_mode); display_print("\n");
    display_print("Reason   : "); display_print(g_forensic_center.comparison.failure_reason); display_print("\n");

    // Panel 10 — Hardware Snapshot
    display_print("--- PANEL 10: HARDWARE SNAPSHOT ---\n");
    display_print("Motherboard : "); display_print(g_forensic_center.hardware.motherboard); display_print("\n");
    display_print("CPU         : "); display_print(g_forensic_center.hardware.cpu); display_print("\n");
    display_print("Vendor      : "); display_print(g_forensic_center.hardware.vendor_name); display_print("\n");
    display_print("========================================\n\n");
}
