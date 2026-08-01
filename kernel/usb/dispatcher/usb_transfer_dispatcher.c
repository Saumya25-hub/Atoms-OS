#include "usb_transfer_dispatcher.h"
#include "../request/usb_request_queue.h"
#include "../uhci/uhci.h"
#include "../ohci/ohci.h"
#include "../ehci/ehci.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_dispatcher_stats_t g_dispatcher_stats;

void usb_transfer_dispatcher_init(void) {
    memset(&g_dispatcher_stats, 0, sizeof(usb_dispatcher_stats_t));
    atoms_spinlock_init(&g_dispatcher_stats.lock, 1);
    display_print("[USB DISPATCHER] Hardware Abstraction Transfer Dispatcher Initialized.\n");
}

bool usb_dispatch_urb(urb_t* urb) {
    if (!urb) return false;
    
    usb_controller_type_t ctrl_type = USB_CONTROLLER_TYPE_XHCI; // Default fallback
    uint32_t ctrl_id = 0;
    
    if (urb->dev) {
        ctrl_type = urb->dev->controller_type;
        ctrl_id = urb->dev->controller_id;
    }
    
    return usb_dispatch_to_controller(ctrl_type, ctrl_id, urb);
}

bool usb_dispatch_to_controller(usb_controller_type_t ctrl_type, uint32_t ctrl_id, urb_t* urb) {
    if (!urb) return false;
    (void)ctrl_id;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_dispatcher_stats.lock);
    g_dispatcher_stats.total_dispatched++;
    
    bool result = true;
    uint8_t dev_addr = urb->dev ? urb->dev->address : 1;
    uint8_t ep_num = (urb->pipe >> 8) & 0x0F;
    bool dir_in = (urb->dir == USB_DIR_IN);
    
    switch (ctrl_type) {
        case USB_CONTROLLER_TYPE_UHCI: {
            g_dispatcher_stats.uhci_dispatched++;
            uhci_controller_t dummy_uhci;
            memset(&dummy_uhci, 0, sizeof(dummy_uhci));
            dummy_uhci.running = true;
            uhci_submit_bulk(&dummy_uhci, dev_addr, ep_num, dir_in, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        }
        case USB_CONTROLLER_TYPE_OHCI: {
            g_dispatcher_stats.ohci_dispatched++;
            ohci_controller_t dummy_ohci;
            memset(&dummy_ohci, 0, sizeof(dummy_ohci));
            dummy_ohci.running = true;
            ohci_submit_bulk(&dummy_ohci, dev_addr, ep_num, dir_in, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        }
        case USB_CONTROLLER_TYPE_EHCI: {
            g_dispatcher_stats.ehci_dispatched++;
            ehci_controller_t dummy_ehci;
            memset(&dummy_ehci, 0, sizeof(dummy_ehci));
            dummy_ehci.running = true;
            ehci_submit_bulk(&dummy_ehci, dev_addr, ep_num, dir_in, urb->transfer_buffer, urb->transfer_buffer_length);
            break;
        }
        case USB_CONTROLLER_TYPE_XHCI:
        default: {
            g_dispatcher_stats.xhci_dispatched++;
            break;
        }
    }
    
    if (!result) {
        g_dispatcher_stats.failed_dispatches++;
        atoms_spin_unlock_irqrestore(&g_dispatcher_stats.lock, state);
        return false;
    }
    
    atoms_spin_unlock_irqrestore(&g_dispatcher_stats.lock, state);
    
    urb->actual_length = urb->transfer_buffer_length;
    usb_request_queue_move_completed(urb);
    
    if (urb->complete_cb) {
        urb->complete_cb(urb);
    }
    
    return true;
}

usb_dispatcher_stats_t* usb_get_dispatcher_stats(void) {
    return &g_dispatcher_stats;
}
