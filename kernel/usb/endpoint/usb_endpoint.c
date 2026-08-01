#include "usb_endpoint.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

#define MAX_GLOBAL_ENDPOINTS 128
static usb_endpoint_t g_ep_registry[MAX_GLOBAL_ENDPOINTS];
static bool g_ep_used[MAX_GLOBAL_ENDPOINTS];
static atoms_spinlock_t g_ep_lock;

void usb_endpoint_manager_init(void) {
    memset(g_ep_registry, 0, sizeof(g_ep_registry));
    memset(g_ep_used, 0, sizeof(g_ep_used));
    atoms_spinlock_init(&g_ep_lock, 1);
    display_print("[USB EP] Endpoint Manager Subsystem Initialized.\n");
}

usb_endpoint_t* usb_endpoint_create(usb_device_t* dev, uint8_t ep_num, usb_direction_t dir, usb_transfer_type_t type, uint16_t max_packet_size, uint8_t interval) {
    if (!dev) return NULL;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_ep_lock);
    
    for (uint32_t i = 0; i < MAX_GLOBAL_ENDPOINTS; i++) {
        if (!g_ep_used[i]) {
            g_ep_used[i] = true;
            usb_endpoint_t* ep = &g_ep_registry[i];
            memset(ep, 0, sizeof(usb_endpoint_t));
            ep->dev = dev;
            ep->ep_number = ep_num & 0x0F;
            ep->dir = dir;
            ep->ep_address = ep->ep_number | (dir == USB_DIR_IN ? 0x80 : 0x00);
            ep->type = type;
            ep->max_packet_size = (max_packet_size > 0) ? max_packet_size : 64;
            ep->interval = interval;
            ep->state = USB_EP_STATE_IDLE;
            ep->toggle = 0;
            atoms_spinlock_init(&ep->lock, 1);
            
            atoms_spin_unlock_irqrestore(&g_ep_lock, state);
            return ep;
        }
    }
    
    atoms_spin_unlock_irqrestore(&g_ep_lock, state);
    return NULL;
}

usb_endpoint_t* usb_endpoint_get(usb_device_t* dev, uint8_t ep_num, usb_direction_t dir) {
    if (!dev) return NULL;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_ep_lock);
    
    uint8_t ep_addr = (ep_num & 0x0F) | (dir == USB_DIR_IN ? 0x80 : 0x00);
    for (uint32_t i = 0; i < MAX_GLOBAL_ENDPOINTS; i++) {
        if (g_ep_used[i] && g_ep_registry[i].dev == dev && g_ep_registry[i].ep_address == ep_addr) {
            atoms_spin_unlock_irqrestore(&g_ep_lock, state);
            return &g_ep_registry[i];
        }
    }
    
    atoms_spin_unlock_irqrestore(&g_ep_lock, state);
    return NULL;
}

bool usb_endpoint_set_state(usb_endpoint_t* ep, usb_endpoint_state_t state) {
    if (!ep) return false;
    atoms_irq_lock_state_t lstate = atoms_spin_lock_irqsave(&ep->lock);
    ep->state = state;
    atoms_spin_unlock_irqrestore(&ep->lock, lstate);
    return true;
}

usb_endpoint_state_t usb_endpoint_get_state(usb_endpoint_t* ep) {
    if (!ep) return USB_EP_STATE_ERROR;
    return ep->state;
}

void usb_endpoint_reset_toggle(usb_endpoint_t* ep) {
    if (!ep) return;
    atoms_irq_lock_state_t lstate = atoms_spin_lock_irqsave(&ep->lock);
    ep->toggle = 0;
    atoms_spin_unlock_irqrestore(&ep->lock, lstate);
}

uint8_t usb_endpoint_get_toggle(usb_endpoint_t* ep) {
    return ep ? ep->toggle : 0;
}

void usb_endpoint_advance_toggle(usb_endpoint_t* ep) {
    if (!ep) return;
    atoms_irq_lock_state_t lstate = atoms_spin_lock_irqsave(&ep->lock);
    ep->toggle ^= 1;
    atoms_spin_unlock_irqrestore(&ep->lock, lstate);
}

bool usb_endpoint_halt(usb_endpoint_t* ep) {
    if (!ep) return false;
    atoms_irq_lock_state_t lstate = atoms_spin_lock_irqsave(&ep->lock);
    ep->state = USB_EP_STATE_HALT;
    atoms_spin_unlock_irqrestore(&ep->lock, lstate);
    display_print("[USB EP] Endpoint Halted\n");
    return true;
}

bool usb_endpoint_clear_halt(usb_endpoint_t* ep) {
    if (!ep) return false;
    atoms_irq_lock_state_t lstate = atoms_spin_lock_irqsave(&ep->lock);
    ep->state = USB_EP_STATE_IDLE;
    ep->toggle = 0;
    atoms_spin_unlock_irqrestore(&ep->lock, lstate);
    display_print("[USB EP] Endpoint Halt Cleared & Toggle Reset\n");
    return true;
}

bool usb_endpoint_recover(usb_endpoint_t* ep) {
    if (!ep) return false;
    display_print("[USB EP] Performing Endpoint Recovery...\n");
    return usb_endpoint_clear_halt(ep);
}
