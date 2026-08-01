#include "../include/usb_hub_port.h"
#include "kernel/drivers/display/display.h"

bool usb_port_reset(usb_hub_port_t* port) {
    if (!port) return false;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&port->lock);
    port->state = PORT_STATE_RESETTING;
    port->status |= PORT_STAT_RESET;
    atoms_spin_unlock_irqrestore(&port->lock, state);
    
    // Simulate reset pulse (50ms) and reset recovery (10ms)
    port->status &= ~PORT_STAT_RESET;
    port->status |= PORT_STAT_ENABLE;
    port->change |= PORT_STAT_C_RESET;
    
    atoms_irq_lock_state_t state2 = atoms_spin_lock_irqsave(&port->lock);
    port->state = PORT_STATE_ENUMERATING;
    atoms_spin_unlock_irqrestore(&port->lock, state2);
    
    return true;
}
