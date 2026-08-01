#include "../include/usb_hub_port.h"
#include "kernel/drivers/display/display.h"

extern bool usb_port_reset(usb_hub_port_t* port);

bool usb_port_recover(usb_hub_port_t* port) {
    if (!port) return false;
    
    display_print("[UHE RECOVERY] Executing Port Recovery on Port #");
    display_print_dec(port->port_num);
    display_print("...\n");
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&port->lock);
    port->is_overcurrent = false;
    port->change = 0;
    port->status &= ~PORT_STAT_OVERCURRENT;
    atoms_spin_unlock_irqrestore(&port->lock, state);
    
    return usb_port_reset(port);
}
