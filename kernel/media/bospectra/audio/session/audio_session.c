#include "audio_session.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_AUDIO_SESSIONS 4U

static BOSPECTRA_AudioSession g_audio_sessions[BOSPECTRA_MAX_AUDIO_SESSIONS];
static bool g_audio_session_subsystem_initialized = false;

void bospectra_audio_session_subsystem_init(void) {
    memset(g_audio_sessions, 0, sizeof(g_audio_sessions));
    g_audio_session_subsystem_initialized = true;
}

void bospectra_audio_session_subsystem_shutdown(void) {
    for (uint32_t i = 0; i < BOSPECTRA_MAX_AUDIO_SESSIONS; i++) {
        if (g_audio_sessions[i].is_active) {
            bospectra_audio_session_close(g_audio_sessions[i].id);
        }
    }
    memset(g_audio_sessions, 0, sizeof(g_audio_sessions));
    g_audio_session_subsystem_initialized = false;
}

BOSPECTRA_AudioSession* bospectra_audio_session_get_by_id(bospectra_audio_session_id_t id) {
    if (!g_audio_session_subsystem_initialized || id == 0 || id > BOSPECTRA_MAX_AUDIO_SESSIONS) return NULL;
    uint32_t idx = id - 1;
    return g_audio_sessions[idx].is_active ? &g_audio_sessions[idx] : NULL;
}

bospectra_error_t bospectra_audio_session_open(const BOSPECTRA_AudioSpec* spec, bospectra_audio_session_id_t* out_session_id) {
    if (!g_audio_session_subsystem_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!spec || !out_session_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint32_t slot = BOSPECTRA_MAX_AUDIO_SESSIONS;
    for (uint32_t i = 0; i < BOSPECTRA_MAX_AUDIO_SESSIONS; i++) {
        if (!g_audio_sessions[i].is_active) {
            slot = i;
            break;
        }
    }
    if (slot >= BOSPECTRA_MAX_AUDIO_SESSIONS) return BOSPECTRA_ERR_BUFFER_OVERFLOW;

    BOSPECTRA_AudioSession* sess = &g_audio_sessions[slot];
    sess->id = slot + 1;
    sess->spec = *spec;
    sess->state = BOSPECTRA_AUDIO_STATE_OPENED;
    bospectra_audio_queue_init(&sess->queue);
    sess->pts_us = 0;
    sess->total_bytes_submitted = 0;
    sess->is_active = true;

    *out_session_id = sess->id;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_session_play(bospectra_audio_session_id_t id) {
    BOSPECTRA_AudioSession* sess = bospectra_audio_session_get_by_id(id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    sess->state = BOSPECTRA_AUDIO_STATE_PLAYING;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_session_pause(bospectra_audio_session_id_t id) {
    BOSPECTRA_AudioSession* sess = bospectra_audio_session_get_by_id(id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    sess->state = BOSPECTRA_AUDIO_STATE_PAUSED;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_session_resume(bospectra_audio_session_id_t id) {
    return bospectra_audio_session_play(id);
}

bospectra_error_t bospectra_audio_session_flush(bospectra_audio_session_id_t id) {
    BOSPECTRA_AudioSession* sess = bospectra_audio_session_get_by_id(id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_audio_queue_flush(&sess->queue);
    sess->pts_us = 0;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_audio_session_close(bospectra_audio_session_id_t id) {
    BOSPECTRA_AudioSession* sess = bospectra_audio_session_get_by_id(id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_audio_queue_flush(&sess->queue);
    memset(sess, 0, sizeof(BOSPECTRA_AudioSession));
    return BOSPECTRA_SUCCESS;
}
