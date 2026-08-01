#include "../include/xhci_bulk.h"
#include "kernel/core/lib/include/string.h"

bool xhci_bulk_in(uint8_t slot_id, uint8_t ep_num, void* buffer, uint32_t len, uint32_t timeout_ms, void (*cb)(xhci_bulk_request_t*), void* user_data) {
    if (!buffer || len == 0) return false;
    
    xhci_bulk_request_t* req = xhci_bulk_request_alloc();
    if (!req) return false;
    
    req->slot_id = slot_id;
    req->ep_num = ep_num;
    req->dir_in = true;
    req->virt_buffer = buffer;
    req->phys_buffer = (uint64_t)buffer; // Identity mapped
    req->transfer_len = len;
    req->timeout_ms = timeout_ms;
    req->completion_cb = cb;
    req->user_data = user_data;
    
    return xhci_bulk_submit(req);
}
