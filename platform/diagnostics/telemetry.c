#include "platform/include/bos_types.h"

typedef struct {
    uint64_t total_events_dispatched;
    uint64_t total_events_coalesced;
    uint64_t total_queue_overflows;
    uint64_t max_dispatch_latency_us;
    uint64_t active_windows_count;
} BOS_PlatformTelemetry;

static BOS_PlatformTelemetry g_telemetry_metrics = {0};

void BOS_Telemetry_RecordEventDispatch(uint64_t latency_us, bool coalesced) {
    g_telemetry_metrics.total_events_dispatched++;
    if (coalesced) g_telemetry_metrics.total_events_coalesced++;
    if (latency_us > g_telemetry_metrics.max_dispatch_latency_us) {
        g_telemetry_metrics.max_dispatch_latency_us = latency_us;
    }
}

void BOS_Telemetry_RecordQueueOverflow(void) {
    g_telemetry_metrics.total_queue_overflows++;
}

void BOS_Telemetry_GetMetrics(BOS_PlatformTelemetry* out_metrics) {
    if (out_metrics) {
        *out_metrics = g_telemetry_metrics;
    }
}
