#include "include/bospectra_sync.h"
#include "clock/master_clock.h"
#include "scheduler/frame_scheduler.h"
#include "drift/drift_detector.h"
#include "diagnostics/sync_diag.h"
#include "tests/sync_tests.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"

static bool g_sync_engine_initialized = false;

bospectra_error_t BOSPECTRA_Sync_Init(void) {
    if (g_sync_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    master_clock_init();
    frame_scheduler_init();
    drift_detector_init();
    bospectra_sync_diag_init();

    g_sync_engine_initialized = true;
    bospectra_log("SYNC_ENGINE", "Audio/Video Synchronization Subsystem Initialized.");

    // Run self-tests and sync loop stress test (disabled for instant boot)
    // bospectra_sync_tests_run_all();

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Sync_Shutdown(void) {
    if (!g_sync_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    bospectra_sync_diag_shutdown();
    drift_detector_shutdown();
    frame_scheduler_shutdown();
    master_clock_shutdown();

    g_sync_engine_initialized = false;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Sync_SetClockSource(bospectra_clock_source_t source) {
    if (!g_sync_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    master_clock_set_source(source);
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Sync_UpdateAudioClock(uint64_t audio_pts_us) {
    if (!g_sync_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    master_clock_update_audio_pts(audio_pts_us);
    return BOSPECTRA_SUCCESS;
}

uint64_t BOSPECTRA_Sync_GetMasterClock(void) {
    if (!g_sync_engine_initialized) return 0;
    return master_clock_get_time_us();
}

bospectra_error_t BOSPECTRA_Sync_SetPlaybackSpeed(uint32_t speed_x100) {
    if (!g_sync_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    master_clock_set_speed(speed_x100);
    return BOSPECTRA_SUCCESS;
}

bospectra_sync_decision_t BOSPECTRA_Sync_EvaluateFrame(uint64_t frame_pts_us) {
    if (!g_sync_engine_initialized) {
        return BOSPECTRA_SYNC_DECISION_PRESENT_IMMEDIATELY;
    }

    uint64_t master_time_us = master_clock_get_time_us();
    drift_detector_update(master_time_us, frame_pts_us);

    bospectra_sync_decision_t decision = frame_scheduler_evaluate(frame_pts_us, master_time_us);
    bospectra_sync_diag_record_decision(decision);

    return decision;
}

void BOSPECTRA_Sync_GetStats(BOSPECTRA_SyncStats* out_stats) {
    bospectra_sync_collect_stats(out_stats);
}

void BOSPECTRA_Sync_DumpDiagnostics(void) {
    bospectra_sync_dump_telemetry();
}
