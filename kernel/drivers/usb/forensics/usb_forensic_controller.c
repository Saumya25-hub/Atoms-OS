#include "usb_forensic_center.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

void usb_forensic_update_controller_dashboard(uint8_t bus, uint8_t slot, uint8_t func, uint32_t usbcmd, uint32_t usbsts, uint32_t dnctrl, uint32_t config, bool running) {
    g_forensic_center.dashboard.bus = bus;
    g_forensic_center.dashboard.slot = slot;
    g_forensic_center.dashboard.func = func;
    g_forensic_center.dashboard.usbcmd = usbcmd;
    g_forensic_center.dashboard.usbsts = usbsts;
    g_forensic_center.dashboard.dnctrl = dnctrl;
    g_forensic_center.dashboard.config = config;

    if (usbsts & 1) { // HCHalted bit
        strcpy(g_forensic_center.dashboard.state, "HALTED");
    } else if (usbsts & (1 << 4)) { // Host System Error
        strcpy(g_forensic_center.dashboard.state, "SYSTEM_ERROR");
    } else if (running) {
        strcpy(g_forensic_center.dashboard.state, "RUNNING");
    } else {
        strcpy(g_forensic_center.dashboard.state, "STOPPED");
    }
}
