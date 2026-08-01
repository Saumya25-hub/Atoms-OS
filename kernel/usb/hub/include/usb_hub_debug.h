#ifndef SIGNATURES_USB_HUB_DEBUG_H
#define SIGNATURES_USB_HUB_DEBUG_H

#include "../../common/usb_common.h"

// UHE Forensic Dump APIs
void uhe_dump_hubs(void);
void uhe_dump_ports(void);
void uhe_dump_topology(void);
void uhe_dump_power(void);
void uhe_dump_events(void);
void uhe_dump_everything(void);

// Telemetry Export API
void uhe_telemetry_init(void);
void uhe_telemetry_dump_json(void);

#endif // SIGNATURES_USB_HUB_DEBUG_H
