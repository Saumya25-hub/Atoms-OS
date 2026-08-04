/*
 * BOSPECTRA V3 — Session Manager Implementation
 * kernel/media/bospectra/manager/session_manager.c
 */

#include "session_manager.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static bool g_session_manager_initialized = false;

void bospectra_session_manager_init(void) {
    playback_session_subsystem_init();
    g_session_manager_initialized = true;
    bospectra_log("SESSION_MANAGER", "BOSPECTRA V3 Session Manager initialized.");
}

void bospectra_session_manager_shutdown(void) {
    playback_session_subsystem_shutdown();
    g_session_manager_initialized = false;
}

bospectra_error_t bospectra_session_create(const char* media_uri, bospectra_playback_session_id_t* out_session_id) {
    if (!g_session_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_session_create(media_uri, out_session_id);
}

PlaybackSessionCtx* bospectra_session_get(bospectra_playback_session_id_t session_id) {
    if (!g_session_manager_initialized) return NULL;
    return playback_session_get_by_id(session_id);
}

bospectra_error_t bospectra_session_destroy(bospectra_playback_session_id_t session_id) {
    if (!g_session_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_session_destroy(session_id);
}

bospectra_error_t bospectra_session_transition_state(bospectra_playback_session_id_t session_id, bospectra_playback_state_t new_state) {
    PlaybackSessionCtx* sess = bospectra_session_get(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;
    return playback_state_transition(&sess->state_machine, new_state);
}
