#include "include/bospectra_playback.h"
#include "session/playback_session.h"
#include "controller/playback_controller.h"
#include "diagnostics/playback_diag.h"
#include "tests/playback_tests.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"

static bool g_playback_engine_initialized = false;

bospectra_error_t BOSPECTRA_Playback_Init(void) {
    if (g_playback_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    playback_session_subsystem_init();
    g_playback_engine_initialized = true;

    bospectra_log("PLAYBACK_ENGINE", "Playback Controller Engine Initialized (Engine Only Mode).");

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Playback_Shutdown(void) {
    if (!g_playback_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    playback_session_subsystem_shutdown();
    g_playback_engine_initialized = false;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Playback_OpenSession(const char* media_uri, bospectra_playback_session_id_t* out_session_id) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_session_create(media_uri, out_session_id);
}

bospectra_error_t BOSPECTRA_Playback_CloseSession(bospectra_playback_session_id_t session_id) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_session_destroy(session_id);
}

bospectra_error_t BOSPECTRA_Playback_Play(bospectra_playback_session_id_t session_id) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_play(session_id);
}

bospectra_error_t BOSPECTRA_Playback_Pause(bospectra_playback_session_id_t session_id) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_pause(session_id);
}

bospectra_error_t BOSPECTRA_Playback_Resume(bospectra_playback_session_id_t session_id) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_resume(session_id);
}

bospectra_error_t BOSPECTRA_Playback_Stop(bospectra_playback_session_id_t session_id) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_stop(session_id);
}

bospectra_error_t BOSPECTRA_Playback_Seek(bospectra_playback_session_id_t session_id, uint64_t target_position_us) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_seek(session_id, target_position_us);
}

bospectra_error_t BOSPECTRA_Playback_SetSpeed(bospectra_playback_session_id_t session_id, uint32_t speed_x100) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_set_speed(session_id, speed_x100);
}

bospectra_error_t BOSPECTRA_Playback_SetLoop(bospectra_playback_session_id_t session_id, bool enable_loop) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_set_loop(session_id, enable_loop);
}

bospectra_error_t BOSPECTRA_Playback_StepFrame(bospectra_playback_session_id_t session_id) {
    if (!g_playback_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    return playback_ctrl_step_frame(session_id);
}

bospectra_playback_state_t BOSPECTRA_Playback_GetState(bospectra_playback_session_id_t session_id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    return sess ? sess->state_machine.current_state : BOSPECTRA_PLAYBACK_STATE_IDLE;
}

bospectra_error_t BOSPECTRA_Playback_GetTimeline(bospectra_playback_session_id_t session_id, BOSPECTRA_TimelineInfo* out_timeline) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess || !out_timeline) return BOSPECTRA_ERR_HANDLE_INVALID;
    return playback_timeline_get_info(&sess->timeline, out_timeline);
}

void BOSPECTRA_Playback_GetStats(bospectra_playback_session_id_t session_id, BOSPECTRA_PlaybackStats* out_stats) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (sess && out_stats) {
        playback_diag_collect_stats(sess, out_stats);
    }
}

void BOSPECTRA_Playback_DumpDiagnostics(bospectra_playback_session_id_t session_id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    playback_diag_dump_telemetry(sess);
}
