#include "../include/usb_hub.h"
#include "../include/usb_hub_debug.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern void usb_hub_registry_init(void);

static bool g_uhe_initialized = false;

void usb_hub_engine_init(void) {
    if (g_uhe_initialized) return;
    
    display_print("[UHE] Initializing USB Hub Engine (UHE) Subsystem...\n");
    usb_hub_registry_init();
    usb_topology_init();
    usb_hub_events_init();
    uhe_telemetry_init();
    
    g_uhe_initialized = true;
    display_print("[UHE] Subsystem Initialized. Topology & Hub Engine Ready.\n");
}

usb_hub_t* usb_hub_create(uint32_t device_id, uint32_t controller_id, bool is_root_hub, uint8_t num_ports) {
    static usb_hub_t pool[USB_MAX_HUBS];
    static uint32_t pool_count = 0;
    static atoms_spinlock_t pool_lock;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pool_lock);
    if (pool_count >= USB_MAX_HUBS) {
        atoms_spin_unlock_irqrestore(&pool_lock, state);
        return NULL;
    }
    
    usb_hub_t* hub = &pool[pool_count++];
    memset(hub, 0, sizeof(usb_hub_t));
    hub->device_id = device_id;
    hub->controller_id = controller_id;
    hub->is_root_hub = is_root_hub;
    hub->num_ports = (num_ports > USB_MAX_HUB_PORTS) ? USB_MAX_HUB_PORTS : num_ports;
    hub->state = HUB_STATE_INIT;
    atoms_spin_unlock_irqrestore(&pool_lock, state);
    
    return hub;
}
