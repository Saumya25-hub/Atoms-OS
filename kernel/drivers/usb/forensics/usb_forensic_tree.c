#include "usb_forensic_center.h"
#include "kernel/core/lib/include/string.h"

void usb_forensic_register_device(uint8_t port, const char* name, uint16_t vid, uint16_t pid, const char* speed, const char* driver) {
    ForensicDeviceTreePanel* tree = &g_forensic_center.tree;
    if (tree->device_count >= FORENSIC_MAX_DEVICES) return;

    ForensicUsbDevice* dev = &tree->devices[tree->device_count++];
    dev->port_num = port;
    strcpy(dev->device_name, name);
    dev->vid = vid;
    dev->pid = pid;
    strcpy(dev->speed, speed);
    strcpy(dev->driver_name, driver);
}

void usb_forensic_update_comparison(bool xhci, bool ehci, bool uhci, bool ohci, const char* used, const char* fallback, const char* reason) {
    ForensicComparisonPanel* c = &g_forensic_center.comparison;
    c->xhci_detected = xhci;
    c->ehci_detected = ehci;
    c->uhci_detected = uhci;
    c->ohci_detected = ohci;
    if (used) strcpy(c->controller_used, used);
    if (fallback) strcpy(c->fallback_mode, fallback);
    if (reason) strcpy(c->failure_reason, reason);
}

void usb_forensic_update_hardware_snapshot(const char* board, const char* cpu, const char* vendor, uint16_t vid, uint16_t did, uint64_t bar0, uint8_t irq, bool msi) {
    ForensicHardwareSnapshot* hw = &g_forensic_center.hardware;
    if (board) strcpy(hw->motherboard, board);
    if (cpu) strcpy(hw->cpu, cpu);
    if (vendor) strcpy(hw->vendor_name, vendor);
    hw->vendor_id = vid;
    hw->device_id = did;
    hw->bar0_phys = bar0;
    hw->irq_line = irq;
    hw->msi_enabled = msi;
}
