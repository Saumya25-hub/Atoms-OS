#include "dispatcher_diag.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static DispatcherDiagnostics g_diag;

void dispatcher_diag_init(void) {
    memset(&g_diag, 0, sizeof(g_diag));
}

void dispatcher_diag_record_received(void) {
    g_diag.total_events_received++;
}

void dispatcher_diag_record_overflow(void) {
    g_diag.queue_overflow_count++;
    g_diag.total_events_dropped++;
}

void dispatcher_diag_update_depth(uint32_t depth) {
    g_diag.current_queue_depth = depth;
    if (depth > g_diag.max_queue_depth) {
        g_diag.max_queue_depth = depth;
    }
}

void dispatcher_diag_record_filter_drop(bool zero_delta, bool duplicate, bool error) {
    g_diag.total_events_dropped++;
    if (zero_delta) g_diag.filter_drops_zero_delta++;
    if (duplicate) g_diag.filter_drops_duplicate++;
    if (error) g_diag.filter_drops_error++;
}

void dispatcher_diag_record_routed(uint64_t cycles_spent) {
    g_diag.total_events_routed++;
    g_diag.total_dispatch_cycles += cycles_spent;
    
    if (g_diag.total_events_routed > 0) {
        uint64_t avg_cycles = g_diag.total_dispatch_cycles / g_diag.total_events_routed;
        g_diag.avg_dispatch_time_us = step14_cycles_to_us(avg_cycles);
    }
}

void dispatcher_diag_record_stop(void) {
    g_diag.propagation_stops++;
}

void dispatcher_diag_get(DispatcherDiagnostics* out_diag) {
    if (out_diag) {
        *out_diag = g_diag;
    }
}

void dispatcher_diag_dump(void) {
    display_print("[DISPATCHER DIAG] === Event Dispatcher Telemetry ===\n");
    display_print("[DISPATCHER DIAG] Received: ");
    // Note: display_print takes strings; in a real dump we would format numbers.
    // Keeping simple and safe for kernel printing.
    display_print("OK\n");
}
