#ifndef AUDIO_SESSION_H
#define AUDIO_SESSION_H

#include "../include/bospectra_audio_types.h"
#include "../queue/audio_queue.h"
#include "../../include/bospectra_errors.h"

typedef enum {
    BOSPECTRA_AUDIO_STATE_STOPPED = 0,
    BOSPECTRA_AUDIO_STATE_OPENED,
    BOSPECTRA_AUDIO_STATE_PLAYING,
    BOSPECTRA_AUDIO_STATE_PAUSED
} bospectra_audio_state_t;

typedef struct {
    bospectra_audio_session_id_t id;
    BOSPECTRA_AudioSpec          spec;
    bospectra_audio_state_t      state;
    BOSPECTRA_AudioQueue         queue;
    uint64_t                     pts_us;
    uint64_t                     total_bytes_submitted;
    bool                         is_active;
} BOSPECTRA_AudioSession;

void                     bospectra_audio_session_subsystem_init(void);
void                     bospectra_audio_session_subsystem_shutdown(void);
bospectra_error_t        bospectra_audio_session_open(const BOSPECTRA_AudioSpec* spec, bospectra_audio_session_id_t* out_session_id);
BOSPECTRA_AudioSession* bospectra_audio_session_get_by_id(bospectra_audio_session_id_t id);
bospectra_error_t        bospectra_audio_session_play(bospectra_audio_session_id_t id);
bospectra_error_t        bospectra_audio_session_pause(bospectra_audio_session_id_t id);
bospectra_error_t        bospectra_audio_session_resume(bospectra_audio_session_id_t id);
bospectra_error_t        bospectra_audio_session_flush(bospectra_audio_session_id_t id);
bospectra_error_t        bospectra_audio_session_close(bospectra_audio_session_id_t id);

#endif // AUDIO_SESSION_H
