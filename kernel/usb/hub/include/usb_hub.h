#ifndef SIGNATURES_USB_HUB_H
#define SIGNATURES_USB_HUB_H

#include "../../common/usb_common.h"
#include "../../core/usb_core.h"
#include "../../urb/usb_urb.h"
#include "usb_hub_descriptor.h"
#include "usb_hub_port.h"
#include "usb_hub_topology.h"
#include "usb_hub_power.h"
#include "usb_hub_events.h"

#define USB_MAX_HUBS 32
#define USB_MAX_HUB_PORTS 16

typedef enum {
    HUB_STATE_UNINITIALIZED = 0,
    HUB_STATE_INIT,
    HUB_STATE_CONFIGURED,
    HUB_STATE_RUNNING,
    HUB_STATE_SUSPENDED,
    HUB_STATE_ERROR
} usb_hub_state_t;

typedef struct usb_hub {
    uint32_t                hub_id;
    uint32_t                device_id;      // Core USB device ID
    uint32_t                controller_id;  // Host Controller ID
    uint8_t                 num_ports;
    usb_hub_state_t         state;
    usb_hub_descriptor_t    descriptor;
    usb_hub_port_t          ports[USB_MAX_HUB_PORTS];
    usb_hub_power_budget_t  power_budget;
    usb_topology_node_t*    topology_node;
    bool                    is_root_hub;
    atoms_spinlock_t        lock;
} usb_hub_t;

typedef struct {
    usb_hub_t        hubs[USB_MAX_HUBS];
    uint32_t         hub_count;
    uint32_t         total_ports;
    uint32_t         active_ports;
    uint32_t         connected_devices;
    atoms_spinlock_t lock;
} usb_hub_registry_t;

// API
void usb_hub_engine_init(void);
usb_hub_t* usb_hub_create(uint32_t device_id, uint32_t controller_id, bool is_root_hub, uint8_t num_ports);
bool usb_hub_register(usb_hub_t* hub);
usb_hub_t* usb_hub_find(uint32_t hub_id);
usb_hub_registry_t* usb_hub_get_registry(void);

#endif // SIGNATURES_USB_HUB_H
