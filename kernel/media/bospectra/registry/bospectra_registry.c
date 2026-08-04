/*
 * BOSPECTRA V3 — Dynamic Multimedia Registry System Implementation
 * kernel/media/bospectra/registry/bospectra_registry.c
 */

#include "bospectra_registry.h"
#include "../debug/bospectra_debug.h"

static bool g_v3_registry_initialized = false;

bospectra_error_t bospectra_v3_registry_init(void) {
    if (g_v3_registry_initialized) return BOSPECTRA_ERR_ALREADY_INITIALIZED;

    bospectra_v3_driver_registry_init();
    g_v3_registry_initialized = true;
    bospectra_log("V3_REGISTRY", "BOSPECTRA V3 Dynamic Multimedia Registry Subsystem Ready.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_v3_registry_shutdown(void) {
    if (!g_v3_registry_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    bospectra_v3_driver_registry_shutdown();
    g_v3_registry_initialized = false;
    return BOSPECTRA_SUCCESS;
}

void bospectra_v3_registry_dump_diagnostics(void) {
    bospectra_v3_driver_registry_dump_telemetry();
}
