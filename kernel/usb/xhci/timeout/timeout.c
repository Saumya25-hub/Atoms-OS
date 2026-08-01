#include "../include/xhci_timeout.h"
#include "../include/xhci_debug.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"

extern void xhci_bulk_request_complete(xhci_bulk_request_t* req, uint32_t comp_code, uint32_t residual_bytes);

void xhci_timeout_checker_run(void) {
    xhci_queue_manager_t* mgr = xhci_get_queue_manager();
    if (!mgr) return;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&mgr->running_queue.lock);
    
    uint64_t now_ms = timer_get_ticks();
    xhci_bulk_request_t* curr = mgr->running_queue.head;
    
    while (curr) {
        xhci_bulk_request_t* next = curr->next;
        if (curr->timeout_ms > 0) {
            uint64_t elapsed = (curr->submit_time_ms == 0) ? (curr->timeout_ms + 1000) : (now_ms - curr->submit_time_ms);
            if (elapsed > curr->timeout_ms) {
                display_print("[XHCI TIMEOUT] Transfer ID "); display_print_dec(curr->request_id); display_print(" timed out!\n");
                xhci_telemetry_record_timeout();
                curr->state = XHCI_REQ_STATE_TIMED_OUT;
            }
        }
        curr = next;
    }
    
    atoms_spin_unlock_irqrestore(&mgr->running_queue.lock, state);
}

bool xhci_transfer_cancel(xhci_bulk_request_t* req) {
    if (!req) return false;
    
    xhci_queue_manager_t* mgr = xhci_get_queue_manager();
    if (xhci_queue_remove(&mgr->pending_queue, req) || xhci_queue_remove(&mgr->running_queue, req)) {
        req->state = XHCI_REQ_STATE_CANCELLED;
        req->completion_code = TRB_COMP_STOPPED;
        xhci_queue_push_tail(&mgr->completed_queue, req);
        if (req->completion_cb) {
            req->completion_cb(req);
        }
        return true;
    }
    return false;
}

bool xhci_endpoint_reset(uint8_t slot_id, uint8_t ep_num, bool dir_in) {
    display_print("[XHCI BTE] Resetting Endpoint: Slot="); display_print_dec(slot_id);
    display_print(" EP="); display_print_dec(ep_num); display_print(dir_in ? " IN\n" : " OUT\n");
    return true;
}

bool xhci_endpoint_stop(uint8_t slot_id, uint8_t ep_num, bool dir_in) {
    return true;
}

bool xhci_set_tr_dequeue_ptr(uint8_t slot_id, uint8_t ep_num, bool dir_in, uint64_t new_dequeue_phys, uint8_t cycle_state) {
    return true;
}
