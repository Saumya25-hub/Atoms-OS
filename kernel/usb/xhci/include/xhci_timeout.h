#ifndef SIGNATURES_XHCI_TIMEOUT_H
#define SIGNATURES_XHCI_TIMEOUT_H

#include "xhci_queue.h"

// Timeout Recovery APIs
void xhci_timeout_checker_run(void);
bool xhci_transfer_cancel(xhci_bulk_request_t* req);
bool xhci_endpoint_reset(uint8_t slot_id, uint8_t ep_num, bool dir_in);
bool xhci_endpoint_stop(uint8_t slot_id, uint8_t ep_num, bool dir_in);
bool xhci_set_tr_dequeue_ptr(uint8_t slot_id, uint8_t ep_num, bool dir_in, uint64_t new_dequeue_phys, uint8_t cycle_state);

#endif // SIGNATURES_XHCI_TIMEOUT_H
