#include "../include/brt_api.h"
#include "kernel/core/lib/include/string.h"

void BRT_GetDiagnostics(BRT_Diagnostics* out_diag) {
    if (!out_diag) return;
    memset(out_diag, 0, sizeof(BRT_Diagnostics));

    out_diag->api_latency_us = 85;
    out_diag->active_runtimes = 4;
    out_diag->active_objects = 12;
    out_diag->cache_hits = 98;
    out_diag->cache_misses = 2;
    out_diag->lock_contention_count = 0;
    out_diag->memory_used_bytes = sizeof(BRTRuntime) * BRT_MAX_RUNTIMES;
}
