#ifndef SIGNATURES_USB_HUB_POWER_H
#define SIGNATURES_USB_HUB_POWER_H

#include "../../common/usb_common.h"
#include "usb_hub_port.h"

// Standard Power Budget Limits (in mA)
#define USB11_PORT_POWER_LIMIT_MA   100
#define USB20_PORT_POWER_LIMIT_MA   500
#define USB30_PORT_POWER_LIMIT_MA   900

typedef struct {
    uint32_t total_budget_ma;
    uint32_t allocated_ma;
    uint32_t overcurrent_count;
    atoms_spinlock_t lock;
} usb_hub_power_budget_t;

// API
void usb_hub_power_budget_init(usb_hub_power_budget_t* pb, uint32_t total_budget_ma);
bool usb_hub_power_request(usb_hub_power_budget_t* pb, uint32_t requested_ma);
void usb_hub_power_release(usb_hub_power_budget_t* pb, uint32_t released_ma);
void usb_hub_handle_overcurrent(usb_hub_power_budget_t* pb, usb_hub_port_t* port);

#endif // SIGNATURES_USB_HUB_POWER_H
