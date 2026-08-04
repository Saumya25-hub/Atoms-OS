#include "sync_diag.h"
#include "../clock/master_clock.h"
#include "../drift/drift_detector.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static uint64_t g_presented_frames = 0;
static uint64_t g_dropped_frames = 0;
static uint64_t g_delayed_frames = 0;
static bool     g_sync_diag_initialized = false;

void bospectra_sync_diag_init(void) {
    g_presented_frames = 0;
    g_dropped_frames = 0;
    g_delayed_frames = 0;
    g_sync_diag_initialized = true;
}

void bospectra_sync_diag_shutdown(void) {
    g_sync_diag_initialized = false;
}

void bospectra_sync_diag_record_decision(bospectra_sync_decision_t decision) {
    if (!g_sync_diag_initialized) return;

    switch (decision) {
        case BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY:
            g_presented_frames++;
            break;
        case BOSPECTRA_SYNC_DECISION_DROP_FRAME:
            g_dropped_frames++;
            break;
        case BOSPECTRA_SYNC_DECISION_WAIT:
            g_delayed_frames++;
            break;
        default:
            break;
    }
}

void bospectra_sync_collect_stats(BOSPECTRA_SyncStats* out_stats) {
    if (!out_stats) return;

    memset(out_stats, 0, sizeof(BOSPECTRA_SyncStats));
    out_stats->master_clock_us = master_clock_get_time_us();
    out_stats->current_drift_us = drift_detector_get_current_drift_us();
    out_stats->avg_drift_us = drift_detector_get_avg_drift_us();
    out_stats->max_drift_us = drift_detector_get_max_drift_us();
    out_stats->frames_presented = g_presented_frames;
    out_stats->frames_dropped = g_dropped_frames;
    out_stats->frames_delayed = g_delayed_frames;
    out_stats->playback_speed_x100 = master_clock_get_speed();
    out_stats->active_clock_source = master_clock_get_source();
}

void bospectra_sync_dump_telemetry(void) {
    display_print("========= BOSPECTRA A/V SYNC ENGINE TELEMETRY =========\n");
    display_print("Master Clock Source:  AUDIO_CLOCK (System RDTSC Fallback)\n");
    display_print("PTS Resolution:       Microseconds (uint64_t)\n");
    display_print("Drift Detector:       ACTIVE (Smooth Compensator)\n");
    display_print("========================================================\n");
}
