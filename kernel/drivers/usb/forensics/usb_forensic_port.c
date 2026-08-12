#include "usb_forensic_center.h"
#include "kernel/core/lib/include/string.h"

void usb_forensic_update_port_status(uint8_t port_num, uint32_t portsc_val, uint8_t assigned_slot_id) {
    if (port_num == 0 || port_num > FORENSIC_MAX_PORTS) return;

    ForensicPortStatus* p = &g_forensic_center.ports.ports[port_num - 1];
    p->port_num = port_num;
    p->ccs = (portsc_val & (1 << 0)) != 0; // Current Connect Status
    p->ped = (portsc_val & (1 << 1)) != 0; // Port Enabled/Disabled
    p->pr  = (portsc_val & (1 << 4)) != 0; // Port Reset

    uint32_t speed = (portsc_val >> 10) & 0x0F;
    if (speed == 1) strcpy(p->speed, "FULL");
    else if (speed == 2) strcpy(p->speed, "LOW");
    else if (speed == 3) strcpy(p->speed, "HIGH");
    else if (speed == 4) strcpy(p->speed, "SUPER");
    else strcpy(p->speed, "UNKNOWN");

    p->slot_id = assigned_slot_id;

    if (!p->ccs) strcpy(p->status, "DISCONNECTED");
    else if (p->pr) strcpy(p->status, "RESETTING");
    else if (p->ped) strcpy(p->status, "ACTIVE");
    else strcpy(p->status, "CONNECTED");

    if (port_num > g_forensic_center.ports.port_count) {
        g_forensic_center.ports.port_count = port_num;
    }
}
