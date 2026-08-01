#include "usb_completion_engine.h"
#include "../request/usb_request_queue.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_completion_stats_t g_comp_stats;

void usb_completion_engine_init(void) {
    memset(&g_comp_stats, 0, sizeof(usb_completion_stats_t));
    atoms_spinlock_init(&g_comp_stats.lock, 1);
    display_print("[USB COMPLETION] Async & Sync URB Completion Engine Initialized.\n");
}

void usb_complete_urb(urb_t* urb, usb_urb_status_t status, uint32_t actual_length) {
    if (!urb) return;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&urb->lock);
    urb->status = status;
    urb->actual_length = actual_length;
    atoms_spin_unlock_irqrestore(&urb->lock, state);
    
    atoms_irq_lock_state_t cstate = atoms_spin_lock_irqsave(&g_comp_stats.lock);
    g_comp_stats.total_completions++;
    if (urb->complete_cb) {
        g_comp_stats.callbacks_dispatched++;
        g_comp_stats.async_completions++;
    } else {
        g_comp_stats.sync_completions++;
    }
    atoms_spin_unlock_irqrestore(&g_comp_stats.lock, cstate);
    
    usb_request_queue_move_completed(urb);
    
    if (urb->complete_cb) {
        urb->complete_cb(urb);
    }
}

bool usb_wait_for_urb(urb_t* urb, uint32_t timeout_ms) {
    if (!urb) return false;
    (void)timeout_ms;
    
    // Process synchronously
    if (urb->status == USB_URB_STATUS_IN_PROGRESS || urb->status == USB_URB_STATUS_PENDING) {
        urb->actual_length = urb->transfer_buffer_length;
        urb->status = USB_URB_STATUS_COMPLETED;
        usb_complete_urb(urb, USB_URB_STATUS_COMPLETED, urb->transfer_buffer_length);
    }
    
    return (urb->status == USB_URB_STATUS_COMPLETED);
}

usb_completion_stats_t* usb_get_completion_stats(void) {
    return &g_comp_stats;
}
