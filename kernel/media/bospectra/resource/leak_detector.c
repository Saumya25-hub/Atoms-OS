/*
 * BOSPECTRA V3 — Leak Detector Implementation
 * kernel/media/bospectra/resource/leak_detector.c
 */

#include "leak_detector.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_LeakDetectorStats g_leak_stats;
static bool                        g_leak_mgr_initialized = false;

void bospectra_leak_detector_init(void) {
    memset(&g_leak_stats, 0, sizeof(g_leak_stats));
    g_leak_mgr_initialized = true;
    bospectra_log("LEAK_DETECTOR", "BOSPECTRA V3 Leak Detector Initialized.");
}

void bospectra_leak_detector_shutdown(void) {
    if (g_leak_stats.currently_alive > 0) {
        g_leak_stats.detected_leaks += g_leak_stats.currently_alive;
        bospectra_log("LEAK_DETECTOR", "Leak Warning: Session Closed With Unreleased Objects!");
    }
    g_leak_mgr_initialized = false;
}

void bospectra_leak_track_alloc(uint32_t res_id) {
    if (!g_leak_mgr_initialized) return;
    g_leak_stats.total_allocated++;
    g_leak_stats.currently_alive++;
    if (g_leak_stats.currently_alive > g_leak_stats.peak_objects) {
        g_leak_stats.peak_objects = g_leak_stats.currently_alive;
    }
    (void)res_id;
}

void bospectra_leak_track_release(uint32_t res_id) {
    if (!g_leak_mgr_initialized) return;
    if (g_leak_stats.currently_alive > 0) {
        g_leak_stats.currently_alive--;
    }
    g_leak_stats.total_released++;
    (void)res_id;
}

BOSPECTRA_LeakDetectorStats bospectra_leak_detector_get_stats(void) {
    return g_leak_stats;
}
