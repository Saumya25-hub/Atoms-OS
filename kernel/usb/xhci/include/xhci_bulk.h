#ifndef SIGNATURES_XHCI_BULK_H
#define SIGNATURES_XHCI_BULK_H

#include "xhci_trb.h"
#include "xhci_ring.h"
#include "xhci_queue.h"
#include "xhci_events.h"
#include "xhci_interrupt.h"
#include "xhci_timeout.h"
#include "xhci_debug.h"

// Initialize the entire Bulk Transfer Engine Subsystem
void xhci_bte_init(void);

// Core Bulk Transfer APIs
xhci_bulk_request_t* xhci_bulk_request_alloc(void);
void xhci_bulk_request_free(xhci_bulk_request_t* req);

bool xhci_bulk_submit(xhci_bulk_request_t* req);
bool xhci_bulk_in(uint8_t slot_id, uint8_t ep_num, void* buffer, uint32_t len, uint32_t timeout_ms, void (*cb)(xhci_bulk_request_t*), void* user_data);
bool xhci_bulk_out(uint8_t slot_id, uint8_t ep_num, const void* buffer, uint32_t len, uint32_t timeout_ms, void (*cb)(xhci_bulk_request_t*), void* user_data);

// Synchronous Bulk APIs for driver simplicity
bool xhci_bulk_transfer_sync(uint8_t slot_id, uint8_t ep_num, bool dir_in, void* buffer, uint32_t len, uint32_t timeout_ms, uint32_t* actual_len);

// Execution of Certification Tests
void xhci_bte_run_certification_tests(void);

#endif // SIGNATURES_XHCI_BULK_H
