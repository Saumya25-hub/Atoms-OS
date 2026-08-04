/*
 * BOSPECTRA V3 — Runtime Health Implementation
 * kernel/media/bospectra/runtime/runtime_health.c
 */

#include "runtime_health.h"
#include "error_dispatcher.h"
#include "../debug/bospectra_debug.h"

static bool g_health_mgr_initialized = false;

void bospectra_runtime_health_init(void) {
    g_health_mgr_initialized = true;
    bospectra_log("RUNTIME_HEALTH", "BOSPECTRA V3 Runtime Health Engine Initialized.");
}

void bospectra_runtime_health_shutdown(void) {
    g_health_mgr_initialized = false;
}

BOSPECTRA_HealthState bospectra_runtime_health_get_state(void) {
    if (!g_health_mgr_initialized) return HEALTH_STATE_FAILED;
    if (bospectra_error_get_total_count() > 10) return HEALTH_STATE_DEGRADED;
    if (bospectra_error_get_total_count() > 0) return HEALTH_STATE_WARNING;
    return HEALTH_STATE_OK;
}

const char* bospectra_health_to_string(BOSPECTRA_HealthState state) {
    switch (state) {
        case HEALTH_STATE_OK:       return "HEALTH_OK (100%)";
        case HEALTH_STATE_WARNING:  return "HEALTH_WARNING";
        case HEALTH_STATE_DEGRADED: return "HEALTH_DEGRADED";
        case HEALTH_STATE_FAILED:   return "HEALTH_FAILED";
        default:                    return "HEALTH_UNKNOWN";
    }
}
