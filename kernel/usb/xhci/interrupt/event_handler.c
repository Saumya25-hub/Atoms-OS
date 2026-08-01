#include "../include/xhci_events.h"
#include "../include/xhci_queue.h"
#include "../include/xhci_debug.h"

extern void xhci_bulk_request_complete(xhci_bulk_request_t* req, uint32_t comp_code, uint32_t residual_bytes);

void xhci_event_process_transfer(const xhci_event_decoded_t* evt) {
    if (!evt) return;
    
    xhci_queue_manager_t* mgr = xhci_get_queue_manager();
    if (!mgr) return;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&mgr->running_queue.lock);
    
    xhci_bulk_request_t* curr = mgr->running_queue.head;
    xhci_bulk_request_t* target = NULL;
    
    uint8_t req_ep_num = evt->endpoint_id / 2;
    bool req_dir_in = (evt->endpoint_id % 2) != 0;
    
    while (curr) {
        if (curr->slot_id == evt->slot_id && curr->ep_num == req_ep_num && curr->dir_in == req_dir_in) {
            target = curr;
            break;
        }
        curr = curr->next;
    }
    
    atoms_spin_unlock_irqrestore(&mgr->running_queue.lock, state);
    
    if (target) {
        xhci_bulk_request_complete(target, evt->completion_code, evt->transfer_length);
    }
}
