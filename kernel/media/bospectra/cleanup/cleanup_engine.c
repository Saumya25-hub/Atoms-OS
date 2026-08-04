/*
 * BOSPECTRA V3 — Cleanup Engine Implementation
 * kernel/media/bospectra/cleanup/cleanup_engine.c
 */

#include "cleanup_engine.h"
#include "cleanup_rules.h"
#include "cleanup_pipeline.h"
#include "cleanup_validator.h"
#include "../debug/bospectra_debug.h"

static bool g_cleanup_engine_initialized = false;

void bospectra_cleanup_engine_init(void) {
    bospectra_cleanup_rules_init();
    bospectra_cleanup_validator_init();
    bospectra_cleanup_pipeline_init();
    g_cleanup_engine_initialized = true;
    bospectra_log("CLEANUP_ENGINE", "BOSPECTRA V3 Panic-Safe Cleanup Engine Initialized.");
}

void bospectra_cleanup_engine_shutdown(void) {
    g_cleanup_engine_initialized = false;
}

bospectra_error_t bospectra_cleanup_destroy_session(PlaybackSessionCtx* sess) {
    if (!g_cleanup_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return bospectra_cleanup_execute_reverse_teardown(sess);
}
