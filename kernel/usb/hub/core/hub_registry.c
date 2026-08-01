#include "../include/usb_hub.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_hub_registry_t g_hub_registry;

void usb_hub_registry_init(void) {
    atoms_spinlock_init(&g_hub_registry.lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_hub_registry.lock);
    memset(&g_hub_registry, 0, sizeof(usb_hub_registry_t));
    atoms_spin_unlock_irqrestore(&g_hub_registry.lock, state);
    display_print("[UHE REGISTRY] Hub Registry Initialized.\n");
}

usb_hub_registry_t* usb_hub_get_registry(void) {
    return &g_hub_registry;
}

bool usb_hub_register(usb_hub_t* hub) {
    if (!hub) return false;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_hub_registry.lock);
    if (g_hub_registry.hub_count >= USB_MAX_HUBS) {
        atoms_spin_unlock_irqrestore(&g_hub_registry.lock, state);
        display_print("[UHE REGISTRY] Error: Maximum hub capacity reached!\n");
        return false;
    }
    
    hub->hub_id = g_hub_registry.hub_count + 1;
    g_hub_registry.hubs[g_hub_registry.hub_count] = *hub;
    usb_hub_t* registered = &g_hub_registry.hubs[g_hub_registry.hub_count];
    g_hub_registry.hub_count++;
    g_hub_registry.total_ports += registered->num_ports;
    atoms_spin_unlock_irqrestore(&g_hub_registry.lock, state);
    
    display_print("[UHE REGISTRY] Registered Hub #");
    display_print_dec(registered->hub_id);
    display_print(" (Ports: ");
    display_print_dec(registered->num_ports);
    display_print(")\n");
    return true;
}

usb_hub_t* usb_hub_find(uint32_t hub_id) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_hub_registry.lock);
    for (uint32_t i = 0; i < g_hub_registry.hub_count; i++) {
        if (g_hub_registry.hubs[i].hub_id == hub_id) {
            atoms_spin_unlock_irqrestore(&g_hub_registry.lock, state);
            return &g_hub_registry.hubs[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_hub_registry.lock, state);
    return NULL;
}
