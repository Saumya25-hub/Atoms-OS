#include "../include/usb_hub_power.h"
#include "kernel/drivers/display/display.h"

void usb_hub_power_budget_init(usb_hub_power_budget_t* pb, uint32_t total_budget_ma) {
    if (!pb) return;
    atoms_spinlock_init(&pb->lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pb->lock);
    pb->total_budget_ma = total_budget_ma;
    pb->allocated_ma = 0;
    pb->overcurrent_count = 0;
    atoms_spin_unlock_irqrestore(&pb->lock, state);
}

bool usb_hub_power_request(usb_hub_power_budget_t* pb, uint32_t requested_ma) {
    if (!pb) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pb->lock);
    if (pb->allocated_ma + requested_ma > pb->total_budget_ma) {
        atoms_spin_unlock_irqrestore(&pb->lock, state);
        display_print("[UHE POWER] Error: Power budget exceeded! (Requested: ");
        display_print_dec(requested_ma);
        display_print("mA, Available: ");
        display_print_dec(pb->total_budget_ma - pb->allocated_ma);
        display_print("mA)\n");
        return false;
    }
    pb->allocated_ma += requested_ma;
    atoms_spin_unlock_irqrestore(&pb->lock, state);
    return true;
}

void usb_hub_power_release(usb_hub_power_budget_t* pb, uint32_t released_ma) {
    if (!pb) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pb->lock);
    if (released_ma <= pb->allocated_ma) {
        pb->allocated_ma -= released_ma;
    } else {
        pb->allocated_ma = 0;
    }
    atoms_spin_unlock_irqrestore(&pb->lock, state);
}

void usb_hub_handle_overcurrent(usb_hub_power_budget_t* pb, usb_hub_port_t* port) {
    if (!pb || !port) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pb->lock);
    pb->overcurrent_count++;
    atoms_spin_unlock_irqrestore(&pb->lock, state);
    
    atoms_irq_lock_state_t pstate = atoms_spin_lock_irqsave(&port->lock);
    port->is_overcurrent = true;
    port->status |= PORT_STAT_OVERCURRENT;
    port->change |= PORT_STAT_C_OVERCURRENT;
    port->state = PORT_STATE_ERROR;
    // Power down port for protection
    port->status &= ~PORT_STAT_POWER;
    atoms_spin_unlock_irqrestore(&port->lock, pstate);
    
    display_print("[UHE POWER] OVER-CURRENT DETECTED! Port #");
    display_print_dec(port->port_num);
    display_print(" shut down for safety.\n");
}
