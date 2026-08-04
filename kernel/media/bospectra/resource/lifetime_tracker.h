/*
 * BOSPECTRA V3 — Lifetime Tracker Subsystem
 * kernel/media/bospectra/resource/lifetime_tracker.h
 *
 * Tracks resource lifetime state machine and validates transitions.
 */

#ifndef BOSPECTRA_V3_LIFETIME_TRACKER_H
#define BOSPECTRA_V3_LIFETIME_TRACKER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LIFETIME_STATE_UNKNOWN = 0,
    LIFETIME_STATE_ALLOCATED,
    LIFETIME_STATE_INITIALIZED,
    LIFETIME_STATE_READY,
    LIFETIME_STATE_ACTIVE,
    LIFETIME_STATE_QUEUED,
    LIFETIME_STATE_PRESENTED,
    LIFETIME_STATE_RELEASED,
    LIFETIME_STATE_DESTROYED
} BOSPECTRA_LifetimeState;

void              bospectra_lifetime_tracker_init(void);
void              bospectra_lifetime_tracker_shutdown(void);

bospectra_error_t bospectra_lifetime_transition(uint32_t res_id, BOSPECTRA_LifetimeState new_state);
BOSPECTRA_LifetimeState bospectra_lifetime_get_state(uint32_t res_id);

#endif /* BOSPECTRA_V3_LIFETIME_TRACKER_H */
