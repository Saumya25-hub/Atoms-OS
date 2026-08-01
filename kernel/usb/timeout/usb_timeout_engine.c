#include "usb_timeout_engine.h"
#include "../request/usb_request_queue.h"
#include "../pipe/usb_pipe.h"
#include "../endpoint/usb_endpoint.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static usb_timeout_stats_t g_timeout_stats;

void usb_timeout_engine_init(void) {
    memset(&g_timeout_stats, 0, sizeof(usb_timeout_stats_t));
    atoms_spinlock_init(&g_timeout_stats.lock, 1);
    display_print("[USB TIMEOUT] Watchdog & Error Recovery Timeout Engine Initialized.\n");
}

bool usb_timeout_handle_urb(urb_t* urb) {
    if (!urb) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_timeout_stats.lock);
    g_timeout_stats.timeouts_detected++;
    
    if (urb->retry_count < urb->max_retries) {
        g_timeout_stats.retries_triggered++;
        atoms_spin_unlock_irqrestore(&g_timeout_stats.lock, state);
        
        display_print("[USB TIMEOUT] URB #");
        display_print_dec(urb->urb_id);
        display_print(" Timed Out. Retrying (Attempt ");
        display_print_dec(urb->retry_count + 1);
        display_print(")\n");
        
        usb_request_queue_move_retry(urb);
        return true;
    } else {
        g_timeout_stats.escalations_triggered++;
        atoms_spin_unlock_irqrestore(&g_timeout_stats.lock, state);
        
        display_print("[USB TIMEOUT] URB #");
        display_print_dec(urb->urb_id);
        display_print(" Exceeded Max Retries. Escalating to Endpoint/Pipe Reset.\n");
        
        usb_request_queue_move_timeout(urb);
        return false;
    }
}

void usb_timeout_engine_tick(void) {
    uint64_t now = timer_get_ticks();
    g_timeout_stats.watchdog_ticks++;
    
    usb_request_queue_system_t* qsys = usb_get_request_queue_system();
    if (!qsys) return;
    
    urb_t* timed_out_list[16];
    uint32_t count = 0;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&qsys->running.lock);
    urb_t* curr = qsys->running.head;
    while (curr && count < 16) {
        urb_t* next = curr->next;
        if (curr->timeout_ms > 0) {
            uint64_t elapsed = (curr->submit_tick == 0) ? (curr->timeout_ms + 1000) : (now - curr->submit_tick);
            if (elapsed > curr->timeout_ms) {
                timed_out_list[count++] = curr;
            }
        }
        curr = next;
    }
    atoms_spin_unlock_irqrestore(&qsys->running.lock, state);
    
    for (uint32_t i = 0; i < count; i++) {
        usb_timeout_handle_urb(timed_out_list[i]);
    }
}

usb_timeout_stats_t* usb_get_timeout_stats(void) {
    return &g_timeout_stats;
}
