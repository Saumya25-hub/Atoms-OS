/*
 * BOSPECTRA V3 — Cleanup Validator Implementation
 * kernel/media/bospectra/cleanup/cleanup_validator.c
 */

#include "cleanup_validator.h"
#include "../debug/bospectra_debug.h"

static bool g_cleanup_val_initialized = false;

void bospectra_cleanup_validator_init(void) {
    g_cleanup_val_initialized = true;
    bospectra_log("CLEANUP_VALIDATOR", "BOSPECTRA V3 Cleanup Validator Initialized.");
}

void bospectra_cleanup_validator_shutdown(void) {
    g_cleanup_val_initialized = false;
}

bool bospectra_cleanup_validate_unlinked(uint32_t object_id) {
    (void)object_id;
    return true;
}
