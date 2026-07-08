#ifndef KERNEL_DISPATCHER_DIAG_H
#define KERNEL_DISPATCHER_DIAG_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 4: Event Dispatcher Diagnostics & Telemetry
// ============================================================================
// Tracks real-time routing speeds, drop counts, queue depth, and propagation.
// ============================================================================

typedef struct {
    uint64_t total_events_received;
    uint64_t total_events_routed;
    uint64_t total_events_dropped;
    uint64_t filter_drops_zero_delta;
    uint64_t filter_drops_duplicate;
    uint64_t filter_drops_error;
    uint64_t propagation_stops;
    uint32_t current_queue_depth;
    uint32_t max_queue_depth;
    uint32_t queue_overflow_count;
    uint64_t total_dispatch_cycles;
    uint32_t avg_dispatch_time_us;
} DispatcherDiagnostics;

// Initialize diagnostics
void dispatcher_diag_init(void);

// Record event ingestion into queue
void dispatcher_diag_record_received(void);

// Record queue overflow
void dispatcher_diag_record_overflow(void);

// Update queue depth
void dispatcher_diag_update_depth(uint32_t depth);

// Record event drop by filter
void dispatcher_diag_record_filter_drop(bool zero_delta, bool duplicate, bool error);

// Record successful routing completion
void dispatcher_diag_record_routed(uint64_t cycles_spent);

// Record propagation stop (event consumed by a tier)
void dispatcher_diag_record_stop(void);

// Get read-only copy of diagnostics
void dispatcher_diag_get(DispatcherDiagnostics* out_diag);

// Dump diagnostics to console/serial
void dispatcher_diag_dump(void);

#endif // KERNEL_DISPATCHER_DIAG_H
