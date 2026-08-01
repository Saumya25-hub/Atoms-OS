#ifndef SIGNATURES_XHCI_DEBUG_H
#define SIGNATURES_XHCI_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include "xhci_queue.h"
#include "xhci_ring.h"

typedef struct {
    uint64_t transfers_submitted;
    uint64_t transfers_completed;
    uint64_t bytes_in;
    uint64_t bytes_out;
    uint64_t retries;
    uint64_t timeouts;
    uint64_t trbs_created;
    uint64_t trbs_recycled;
    uint64_t interrupts_handled;
    uint64_t total_latency_us;
    uint64_t max_latency_us;
    uint32_t active_queues;
    uint64_t ring_wraps;
    uint64_t controller_errors;
} xhci_telemetry_t;

// Telemetry APIs
void xhci_telemetry_init(void);
xhci_telemetry_t* xhci_telemetry_get(void);
void xhci_telemetry_record_submit(uint32_t len, bool is_in);
void xhci_telemetry_record_complete(uint32_t len, bool is_in, uint64_t latency_us);
void xhci_telemetry_record_error(void);
void xhci_telemetry_record_timeout(void);

// AI Forensic Debug APIs
void xhci_dump_transfer(const xhci_bulk_request_t* req);
void xhci_dump_ring(const xhci_transfer_ring_t* ring, const char* ring_name);
void xhci_dump_queue(const xhci_request_queue_t* queue, const char* queue_name);
void xhci_dump_endpoint(uint8_t slot_id, uint8_t ep_num, bool dir_in);
void xhci_dump_controller(void);
void xhci_dump_statistics(void);
void xhci_dump_everything(void);

#endif // SIGNATURES_XHCI_DEBUG_H
