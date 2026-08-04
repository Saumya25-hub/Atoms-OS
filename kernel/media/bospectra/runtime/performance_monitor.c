/*
 * BOSPECTRA V3 — Performance Monitor Implementation
 * kernel/media/bospectra/runtime/performance_monitor.c
 */

#include "performance_monitor.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_PerformanceStats g_perf_stats;
static bool                       g_perf_mgr_initialized = false;

void bospectra_performance_monitor_init(uint32_t target_fps) {
    memset(&g_perf_stats, 0, sizeof(g_perf_stats));
    g_perf_stats.target_fps = target_fps > 0 ? target_fps : 30;
    g_perf_stats.current_fps = g_perf_stats.target_fps;
    g_perf_stats.average_fps = g_perf_stats.target_fps;
    g_perf_mgr_initialized = true;
    bospectra_log("PERF_MONITOR", "BOSPECTRA V3 Performance Monitor Initialized.");
}

void bospectra_performance_monitor_shutdown(void) {
    g_perf_mgr_initialized = false;
}

void bospectra_performance_record_frame(uint64_t pts_us) {
    if (!g_perf_mgr_initialized) return;
    (void)pts_us;
}

BOSPECTRA_PerformanceStats bospectra_performance_get_stats(void) {
    return g_perf_stats;
}
