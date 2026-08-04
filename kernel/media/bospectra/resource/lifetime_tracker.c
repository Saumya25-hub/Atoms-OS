/*
 * BOSPECTRA V3 — Lifetime Tracker Implementation
 * kernel/media/bospectra/resource/lifetime_tracker.c
 */

#include "lifetime_tracker.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_LIFETIME_TRACK_ENTRIES 256

typedef struct {
    uint32_t                resource_id;
    BOSPECTRA_LifetimeState state;
    bool                    is_valid;
} BOSPECTRA_LifetimeEntry;

static BOSPECTRA_LifetimeEntry g_lifetime_table[BOSPECTRA_MAX_LIFETIME_TRACK_ENTRIES];
static bool                    g_lifetime_mgr_initialized = false;

void bospectra_lifetime_tracker_init(void) {
    memset(g_lifetime_table, 0, sizeof(g_lifetime_table));
    g_lifetime_mgr_initialized = true;
    bospectra_log("LIFETIME_TRACKER", "BOSPECTRA V3 Lifetime Tracker Initialized.");
}

void bospectra_lifetime_tracker_shutdown(void) {
    g_lifetime_mgr_initialized = false;
}

bospectra_error_t bospectra_lifetime_transition(uint32_t res_id, BOSPECTRA_LifetimeState new_state) {
    if (!g_lifetime_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_LIFETIME_TRACK_ENTRIES; i++) {
        if (g_lifetime_table[i].is_valid && g_lifetime_table[i].resource_id == res_id) {
            g_lifetime_table[i].state = new_state;
            return BOSPECTRA_SUCCESS;
        }
    }

    for (size_t i = 0; i < BOSPECTRA_MAX_LIFETIME_TRACK_ENTRIES; i++) {
        if (!g_lifetime_table[i].is_valid) {
            g_lifetime_table[i].resource_id = res_id;
            g_lifetime_table[i].state = new_state;
            g_lifetime_table[i].is_valid = true;
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_BUFFER_OVERFLOW;
}

BOSPECTRA_LifetimeState bospectra_lifetime_get_state(uint32_t res_id) {
    if (!g_lifetime_mgr_initialized) return LIFETIME_STATE_UNKNOWN;
    for (size_t i = 0; i < BOSPECTRA_MAX_LIFETIME_TRACK_ENTRIES; i++) {
        if (g_lifetime_table[i].is_valid && g_lifetime_table[i].resource_id == res_id) {
            return g_lifetime_table[i].state;
        }
    }
    return LIFETIME_STATE_UNKNOWN;
}
