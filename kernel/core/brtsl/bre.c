#include "kernel/core/brtsl/bre.h"
#include <stddef.h>

// Extern timer for telemetry
extern uint64_t step14_rdtsc(void);
extern uint64_t step14_cycles_to_us(uint64_t cycles);

typedef struct {
    BreServiceCallback callback;
    uint32_t default_budget;
    bool is_active;
} BreServiceDef;

static BreServiceDef g_bre_services[BRE_SERVICE_MAX] = {0};
static volatile uint32_t g_bre_pending_mask = 0;
static BRETelemetry g_bre_telemetry = {0};

void BRE_Init(void) {
    g_bre_pending_mask = 0;
    for (int i = 0; i < BRE_SERVICE_MAX; i++) {
        g_bre_services[i].callback = NULL;
        g_bre_services[i].default_budget = 0;
        g_bre_services[i].is_active = false;
    }
}

void BRE_RegisterService(BreServiceId id, BreServiceCallback callback, uint32_t default_budget) {
    if (id >= BRE_SERVICE_MAX) return;
    g_bre_services[id].callback = callback;
    g_bre_services[id].default_budget = default_budget;
    g_bre_services[id].is_active = true;
}

void BRE_DisableService(BreServiceId id) {
    if (id >= BRE_SERVICE_MAX) return;
    g_bre_services[id].is_active = false;
}

void BRE_Signal(BreServiceId id) {
    if (id >= BRE_SERVICE_MAX) return;
    if (!g_bre_services[id].is_active) return;
    
    uint32_t bit = (1 << id);
    
    // Atomic OR using built-in
    uint32_t old = __sync_fetch_and_or(&g_bre_pending_mask, bit);
    
    g_bre_telemetry.total_signals[id]++;
    if (old & bit) {
        // Was already pending, coalesced
        g_bre_telemetry.coalesced_signals[id]++;
    }
}

void BRE_DispatchPending(void) {
    // Atomically claim all pending work
    uint32_t pending = __sync_lock_test_and_set(&g_bre_pending_mask, 0);
    
    if (pending == 0) return;
    
    // Process services in priority order (0 = highest)
    for (int i = 0; i < BRE_SERVICE_MAX; i++) {
        uint32_t bit = (1 << i);
        if (pending & bit) {
            if (g_bre_services[i].is_active && g_bre_services[i].callback != NULL) {
                uint64_t start_cycles = step14_rdtsc();
                
                bool more_work = g_bre_services[i].callback(g_bre_services[i].default_budget);
                
                uint64_t end_cycles = step14_rdtsc();
                uint32_t dur_us = (uint32_t)step14_cycles_to_us(end_cycles - start_cycles);
                
                g_bre_telemetry.total_dispatches[i]++;
                g_bre_telemetry.total_dispatch_time_us[i] += dur_us;
                
                if (dur_us > g_bre_telemetry.max_dispatch_us[i]) {
                    g_bre_telemetry.max_dispatch_us[i] = dur_us;
                }
                
                if (more_work) {
                    g_bre_telemetry.budget_exhaustions[i]++;
                    // Resignal for next dispatch
                    BRE_Signal((BreServiceId)i);
                }
            }
        }
    }
}

void bre_get_telemetry(BRETelemetry* out_stats) {
    if (out_stats) {
        *out_stats = g_bre_telemetry;
    }
}
