/**
 * @file profiler.c
 * @brief Performance Profiler Engine (PPE) Core Coordinator
 * @status Phase 1 Architecture Core Implementation
 */

#include "../include/profiler.h"
#include "kernel/drivers/display/display.h"

#if BOS_ENABLE_PROFILER

/* Global Profiler Engine State */
static bool             g_profiler_initialized = false;
static uint64_t         g_tsc_frequency_hz     = 2000000000ULL; // Calibrated 2.0 GHz baseline
BOS_FrameMetrics        g_current_frame_metrics;


/* External Hooks for Submodule Delegates */
extern void bos_prof_frame_submodule_begin(BOS_FrameMetrics* frame);
extern void bos_prof_frame_submodule_end(BOS_FrameMetrics* frame);
extern void bos_prof_hotspot_record(const char* func_name, uint64_t elapsed_cycles);
extern void bos_prof_report_submodule_check(const BOS_FrameMetrics* frame);

void bos_profiler_init(void) {
    if (g_profiler_initialized) return;
    
    // Zero out frame metrics snapshot
    for (uint32_t i = 0; i < sizeof(g_current_frame_metrics); i++) {
        ((uint8_t*)&g_current_frame_metrics)[i] = 0;
    }
    
    g_profiler_initialized = true;
    display_print("[PPE] Performance Profiler Engine (Phase 1 Architecture) Initialized.\n");
}

void bos_profiler_shutdown(void) {
    g_profiler_initialized = false;
}

void bos_profiler_frame_begin(void) {
    if (!g_profiler_initialized) bos_profiler_init();
    
    uint64_t tsc = bos_profiler_rdtsc();
    g_current_frame_metrics.frame_id++;
    g_current_frame_metrics.start_tsc = tsc;
    
    // Reset frame transient accumulators
    g_current_frame_metrics.bytes_copied = 0;
    g_current_frame_metrics.vram_bytes_copied = 0;
    g_current_frame_metrics.dirty_rect_area = 0;
    g_current_frame_metrics.total_dirty_rects = 0;
    g_current_frame_metrics.framebuffer_writes = 0;
    g_current_frame_metrics.alloc_count = 0;
    g_current_frame_metrics.free_count = 0;
    g_current_frame_metrics.temp_alloc_count = 0;
    
    bos_prof_frame_submodule_begin(&g_current_frame_metrics);
}

void bos_profiler_frame_end(void) {
    if (!g_profiler_initialized) return;
    
    uint64_t tsc = bos_profiler_rdtsc();
    g_current_frame_metrics.end_tsc = tsc;
    
    uint64_t elapsed_cycles = tsc - g_current_frame_metrics.start_tsc;
    g_current_frame_metrics.frame_time_us = (uint32_t)((elapsed_cycles * 1000000ULL) / g_tsc_frequency_hz);
    
    bos_prof_frame_submodule_end(&g_current_frame_metrics);
    bos_prof_report_submodule_check(&g_current_frame_metrics);
}

BOS_ProfScopeToken bos_profiler_scope_enter(const char* func_name) {
    BOS_ProfScopeToken token;
    token.func_name = func_name ? func_name : "unknown_function";
    token.start_tsc = bos_profiler_rdtsc();
    return token;
}

void bos_profiler_scope_exit(BOS_ProfScopeToken* token) {
    if (!token || !token->start_tsc) return;
    uint64_t end_tsc = bos_profiler_rdtsc();
    uint64_t elapsed = end_tsc - token->start_tsc;
    bos_prof_hotspot_record(token->func_name, elapsed);
}

#endif /* BOS_ENABLE_PROFILER */
