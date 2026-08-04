/*
 * BOSPECTRA V3 — Statistics Manager Implementation
 * kernel/media/bospectra/runtime/statistics_manager.c
 */

#include "statistics_manager.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_GlobalStatistics g_global_stats;
static bool                       g_stats_mgr_initialized = false;

void bospectra_statistics_manager_init(void) {
    memset(&g_global_stats, 0, sizeof(g_global_stats));
    g_stats_mgr_initialized = true;
    bospectra_log("STATS_MANAGER", "BOSPECTRA V3 Statistics Manager Initialized.");
}

void bospectra_statistics_manager_shutdown(void) {
    g_stats_mgr_initialized = false;
}

BOSPECTRA_GlobalStatistics bospectra_statistics_get_global(void) {
    return g_global_stats;
}
