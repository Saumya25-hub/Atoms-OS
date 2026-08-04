#ifndef BOSPECTRA_PLAYBACK_H
#define BOSPECTRA_PLAYBACK_H

#include "../../include/bospectra_types.h"
#include "bospectra_playback_types.h"

typedef struct {
    bospectra_playback_state_t current_state;
    uint64_t                   current_position_us;
    uint64_t                   duration_us;
    uint32_t                   speed_x100;
    bool                       is_looping;
    uint32_t                   loop_count;
    uint64_t                   total_frames_decoded;
    uint64_t                   total_frames_rendered;
    uint64_t                   total_audio_bytes;
    uint32_t                   session_errors;
} BOSPECTRA_PlaybackStats;

// Master Playback Controller Engine Lifecycle APIs
bospectra_error_t BOSPECTRA_Playback_Init(void);
bospectra_error_t BOSPECTRA_Playback_Shutdown(void);

// Session Lifecycle APIs
bospectra_error_t BOSPECTRA_Playback_OpenSession(const char* media_uri, bospectra_playback_session_id_t* out_session_id);
bospectra_error_t BOSPECTRA_Playback_CloseSession(bospectra_playback_session_id_t session_id);

// Playback Control APIs
bospectra_error_t BOSPECTRA_Playback_Play(bospectra_playback_session_id_t session_id);
bospectra_error_t BOSPECTRA_Playback_Pause(bospectra_playback_session_id_t session_id);
bospectra_error_t BOSPECTRA_Playback_Resume(bospectra_playback_session_id_t session_id);
bospectra_error_t BOSPECTRA_Playback_Stop(bospectra_playback_session_id_t session_id);
bospectra_error_t BOSPECTRA_Playback_Seek(bospectra_playback_session_id_t session_id, uint64_t target_position_us);
bospectra_error_t BOSPECTRA_Playback_SetSpeed(bospectra_playback_session_id_t session_id, uint32_t speed_x100);
bospectra_error_t BOSPECTRA_Playback_SetLoop(bospectra_playback_session_id_t session_id, bool enable_loop);
bospectra_error_t BOSPECTRA_Playback_StepFrame(bospectra_playback_session_id_t session_id);

// Timeline & State Queries
bospectra_playback_state_t BOSPECTRA_Playback_GetState(bospectra_playback_session_id_t session_id);
bospectra_error_t          BOSPECTRA_Playback_GetTimeline(bospectra_playback_session_id_t session_id, BOSPECTRA_TimelineInfo* out_timeline);
void                       BOSPECTRA_Playback_GetStats(bospectra_playback_session_id_t session_id, BOSPECTRA_PlaybackStats* out_stats);
void                       BOSPECTRA_Playback_DumpDiagnostics(bospectra_playback_session_id_t session_id);

#endif // BOSPECTRA_PLAYBACK_H
