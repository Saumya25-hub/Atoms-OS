/*
 * BOSPECTRA V3 — Telemetry Engine Implementation
 * kernel/media/bospectra/runtime/telemetry_engine.c
 */

#include "telemetry_engine.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_TelemetryData g_telemetry_data;
static bool                    g_telemetry_mgr_initialized = false;

void bospectra_telemetry_engine_init(void) {
    memset(&g_telemetry_data, 0, sizeof(g_telemetry_data));
    g_telemetry_mgr_initialized = true;
    bospectra_log("TELEMETRY_ENGINE", "BOSPECTRA V3 Telemetry Engine Initialized.");
}

void bospectra_telemetry_engine_shutdown(void) {
    g_telemetry_mgr_initialized = false;
}

void bospectra_telemetry_record_frame_decode(uint32_t duration_us) {
    if (!g_telemetry_mgr_initialized) return;
    g_telemetry_data.frames_decoded++;
    g_telemetry_data.decode_time_us = duration_us;
}

void bospectra_telemetry_record_frame_present(uint32_t duration_us) {
    if (!g_telemetry_mgr_initialized) return;
    g_telemetry_data.frames_presented++;
    g_telemetry_data.present_time_us = duration_us;
}

BOSPECTRA_TelemetryData bospectra_telemetry_get_data(void) {
    return g_telemetry_data;
}
