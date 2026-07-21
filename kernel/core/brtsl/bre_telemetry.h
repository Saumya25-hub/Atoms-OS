#ifndef BRE_TELEMETRY_H
#define BRE_TELEMETRY_H

#include <stdint.h>

// RAM-Only Telemetry for the BOS Reflex Engine
typedef struct {
    uint32_t total_signals[32];
    uint32_t coalesced_signals[32];
    uint32_t total_dispatches[32];
    uint32_t budget_exhaustions[32];
    
    uint32_t max_dispatch_us[32];
    uint64_t total_dispatch_time_us[32];
    
    uint32_t invariant_failures[32];
} BRETelemetry;

void bre_get_telemetry(BRETelemetry* out_stats);

#endif // BRE_TELEMETRY_H
