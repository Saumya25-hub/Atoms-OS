/*
 * BOSPECTRA V3 — Session Manager
 * kernel/media/bospectra/manager/session_manager.h
 *
 * Session IDs, state machine, session lifetime, cleanup, and validation.
 */

#ifndef BOSPECTRA_SESSION_MANAGER_H
#define BOSPECTRA_SESSION_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../playback/session/playback_session.h"
#include <stdint.h>
#include <stdbool.h>

void bospectra_session_manager_init(void);
void bospectra_session_manager_shutdown(void);

bospectra_error_t bospectra_session_create(const char* media_uri, bospectra_playback_session_id_t* out_session_id);
PlaybackSessionCtx* bospectra_session_get(bospectra_playback_session_id_t session_id);
bospectra_error_t bospectra_session_destroy(bospectra_playback_session_id_t session_id);
bospectra_error_t bospectra_session_transition_state(bospectra_playback_session_id_t session_id, bospectra_playback_state_t new_state);

#endif /* BOSPECTRA_SESSION_MANAGER_H */
