#include "../include/bsom_api.h"
#include "kernel/core/lib/include/string.h"

void BSOM_GetDiagnostics(BSOM_Diagnostics* out_diag) {
    if (!out_diag) return;
    memset(out_diag, 0, sizeof(BSOM_Diagnostics));
    out_diag->active_objects = 25;
    out_diag->active_handles = 8;
    out_diag->cache_hits = 320;
    out_diag->cache_misses = 5;
    out_diag->api_latency_us = 42;
    out_diag->memory_used_bytes = sizeof(BSOMObject) * BSOM_MAX_OBJECTS;
}
