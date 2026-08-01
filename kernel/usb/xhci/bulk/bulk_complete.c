#include "../include/xhci_bulk.h"
#include "kernel/core/timer/include/timer.h"

void xhci_bulk_request_complete(xhci_bulk_request_t* req, uint32_t comp_code, uint32_t residual_bytes) {
    if (!req) return;
    
    xhci_queue_manager_t* mgr = xhci_get_queue_manager();
    xhci_queue_remove(&mgr->running_queue, req);
    
    req->completion_code = comp_code;
    req->actual_len = req->transfer_len - residual_bytes;
    
    uint64_t end_time = timer_get_ticks();
    uint64_t latency_us = (end_time - req->submit_time_ms) * 1000;
    
    if (comp_code == TRB_COMP_SUCCESS || comp_code == TRB_COMP_SHORT_PACKET) {
        req->state = XHCI_REQ_STATE_COMPLETED;
        xhci_queue_push_tail(&mgr->completed_queue, req);
        xhci_telemetry_record_complete(req->actual_len, req->dir_in, latency_us);
    } else {
        req->state = XHCI_REQ_STATE_ERROR;
        xhci_telemetry_record_error();
    }
    
    if (req->completion_cb) {
        req->completion_cb(req);
    }
}
