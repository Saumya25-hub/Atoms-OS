#include "usb_urb.h"
#include "../request/usb_request_queue.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"

static urb_pool_t g_urb_pool;
static uint32_t g_next_urb_id = 1000;

void usb_urb_engine_init(void) {
    memset(&g_urb_pool, 0, sizeof(urb_pool_t));
    atoms_spinlock_init(&g_urb_pool.lock, 1);
    display_print("[USB URB] URB Engine & Pre-Allocated Pool (Size: 512) Initialized.\n");
}

urb_t* usb_alloc_urb(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_urb_pool.lock);
    for (uint32_t i = 0; i < MAX_URB_POOL_SIZE; i++) {
        if (!g_urb_pool.in_use[i]) {
            g_urb_pool.in_use[i] = true;
            g_urb_pool.active_urbs++;
            g_urb_pool.allocated_count++;
            
            urb_t* urb = &g_urb_pool.pool[i];
            memset(urb, 0, sizeof(urb_t));
            urb->urb_id = g_next_urb_id++;
            urb->ref_count = 1;
            urb->status = USB_URB_STATUS_PENDING;
            urb->max_retries = 3;
            urb->timeout_ms = 5000;
            atoms_spinlock_init(&urb->lock, 1);
            
            atoms_spin_unlock_irqrestore(&g_urb_pool.lock, state);
            return urb;
        }
    }
    atoms_spin_unlock_irqrestore(&g_urb_pool.lock, state);
    return NULL;
}

void usb_free_urb(urb_t* urb) {
    if (!urb) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_urb_pool.lock);
    uint64_t idx = ((uint64_t)urb - (uint64_t)g_urb_pool.pool) / sizeof(urb_t);
    if (idx < MAX_URB_POOL_SIZE && g_urb_pool.in_use[idx]) {
        g_urb_pool.in_use[idx] = false;
        if (g_urb_pool.active_urbs > 0) g_urb_pool.active_urbs--;
    }
    atoms_spin_unlock_irqrestore(&g_urb_pool.lock, state);
}

urb_t* usb_clone_urb(urb_t* src_urb) {
    if (!src_urb) return NULL;
    urb_t* clone = usb_alloc_urb();
    if (!clone) return NULL;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&src_urb->lock);
    uint32_t new_id = clone->urb_id;
    *clone = *src_urb;
    clone->urb_id = new_id;
    clone->ref_count = 1;
    clone->status = USB_URB_STATUS_PENDING;
    clone->next = NULL;
    clone->prev = NULL;
    atoms_spinlock_init(&clone->lock, 1);
    atoms_spin_unlock_irqrestore(&src_urb->lock, state);
    
    return clone;
}

void usb_get_urb(urb_t* urb) {
    if (!urb) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&urb->lock);
    urb->ref_count++;
    atoms_spin_unlock_irqrestore(&urb->lock, state);
}

void usb_put_urb(urb_t* urb) {
    if (!urb) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&urb->lock);
    if (urb->ref_count > 0) urb->ref_count--;
    uint32_t count = urb->ref_count;
    atoms_spin_unlock_irqrestore(&urb->lock, state);
    
    if (count == 0) {
        usb_free_urb(urb);
    }
}

bool usb_submit_urb(urb_t* urb) {
    if (!urb) return false;
    urb->submit_tick = timer_get_ticks();
    urb->status = USB_URB_STATUS_IN_PROGRESS;
    return usb_request_queue_enqueue_pending(urb);
}

bool usb_cancel_urb(urb_t* urb) {
    if (!urb) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&urb->lock);
    urb->status = USB_URB_STATUS_CANCELLED;
    atoms_spin_unlock_irqrestore(&urb->lock, state);
    
    atoms_irq_lock_state_t pstate = atoms_spin_lock_irqsave(&g_urb_pool.lock);
    g_urb_pool.cancelled_count++;
    atoms_spin_unlock_irqrestore(&g_urb_pool.lock, pstate);
    
    return usb_request_queue_cancel(urb);
}

urb_pool_t* usb_get_urb_pool(void) {
    return &g_urb_pool;
}
