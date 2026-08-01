#include "../include/xhci_queue.h"
#include "../include/xhci_ring.h"
#include "../include/xhci_interrupt.h"
#include "../include/xhci_debug.h"
#include "kernel/core/sync/spinlock.h"

extern xhci_transfer_ring_t g_bte_ep_rings[256][32];
extern bool g_bte_ep_configured[256][32];

void xhci_scheduler_dispatch_pending(void) {
    xhci_queue_manager_t* mgr = xhci_get_queue_manager();
    if (!mgr) return;
    
    xhci_bulk_request_t* req = xhci_queue_pop_head(&mgr->pending_queue);
    while (req) {
        req->state = XHCI_REQ_STATE_RUNNING;
        xhci_queue_push_tail(&mgr->running_queue, req);
        
        uint8_t dci = (req->ep_num * 2) + (req->dir_in ? 1 : 0);
        xhci_transfer_ring_t* ring = &g_bte_ep_rings[req->slot_id][dci];
        
        // Enqueue request TRB onto xHCI Endpoint Transfer Ring
        uint32_t flags = TRB_CTRL_IOC;
        if (req->dir_in) flags |= TRB_CTRL_ISP;
        
        uint32_t trb_idx = 0;
        xhci_transfer_ring_enqueue_normal(ring, req->phys_buffer, req->transfer_len, flags, &trb_idx);
        
        // Ring Endpoint Doorbell to trigger hardware processing
        xhci_doorbell_ring(req->slot_id, dci);
        
        xhci_telemetry_record_submit(req->transfer_len, req->dir_in);
        
        req = xhci_queue_pop_head(&mgr->pending_queue);
    }
}
