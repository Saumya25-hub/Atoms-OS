#include "playback_controller.h"
#include "../session/playback_session.h"
#include "../../frame_memory/frame_pool/frame_pool.h"

bospectra_error_t playback_ctrl_play(bospectra_playback_session_id_t session_id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_error_t err = playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_PLAYING);
    if (err == BOSPECTRA_SUCCESS) {
        if (sess->audio_session_id != 0) {
            BOSPECTRA_Audio_Play(sess->audio_session_id);
        }
        playback_events_post(&sess->event_bus, BOSPECTRA_EVENT_PLAYBACK_STARTED, session_id);
    }
    return err;
}

bospectra_error_t playback_ctrl_pause(bospectra_playback_session_id_t session_id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_error_t err = playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_PAUSED);
    if (err == BOSPECTRA_SUCCESS) {
        if (sess->audio_session_id != 0) {
            BOSPECTRA_Audio_Pause(sess->audio_session_id);
        }
        playback_events_post(&sess->event_bus, BOSPECTRA_EVENT_PLAYBACK_PAUSED, session_id);
    }
    return err;
}

bospectra_error_t playback_ctrl_resume(bospectra_playback_session_id_t session_id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_error_t err = playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_PLAYING);
    if (err == BOSPECTRA_SUCCESS) {
        if (sess->audio_session_id != 0) {
            BOSPECTRA_Audio_Resume(sess->audio_session_id);
        }
        playback_events_post(&sess->event_bus, BOSPECTRA_EVENT_PLAYBACK_RESUMED, session_id);
    }
    return err;
}

bospectra_error_t playback_ctrl_stop(bospectra_playback_session_id_t session_id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_error_t err = playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_STOPPED);
    if (err == BOSPECTRA_SUCCESS) {
        if (sess->audio_session_id != 0) {
            BOSPECTRA_Audio_Flush(sess->audio_session_id);
        }
        playback_timeline_update_position(&sess->timeline, 0);
        playback_events_post(&sess->event_bus, BOSPECTRA_EVENT_PLAYBACK_STOPPED, session_id);
    }
    return err;
}

bospectra_error_t playback_ctrl_seek(bospectra_playback_session_id_t session_id, uint64_t target_position_us) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_playback_state_t prev = sess->state_machine.current_state;
    playback_events_post(&sess->event_bus, BOSPECTRA_EVENT_SEEK_STARTED, (uint32_t)(target_position_us / 1000ULL));

    playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_SEEKING);
    playback_timeline_seek(&sess->timeline, target_position_us);

    if (sess->audio_session_id != 0) {
        BOSPECTRA_Audio_Flush(sess->audio_session_id);
    }
    BOSPECTRA_Sync_UpdateAudioClock(target_position_us);

    playback_state_transition(&sess->state_machine, prev);
    playback_events_post(&sess->event_bus, BOSPECTRA_EVENT_SEEK_FINISHED, (uint32_t)(target_position_us / 1000ULL));

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t playback_ctrl_set_speed(bospectra_playback_session_id_t session_id, uint32_t speed_x100) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    playback_timeline_set_speed(&sess->timeline, speed_x100);
    BOSPECTRA_Sync_SetPlaybackSpeed(speed_x100);
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t playback_ctrl_set_loop(bospectra_playback_session_id_t session_id, bool enable_loop) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    playback_timeline_set_loop(&sess->timeline, enable_loop);
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t playback_ctrl_step_frame(bospectra_playback_session_id_t session_id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(session_id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    // Simulate single-frame step presentation
    BOSFrame* dummy_frame = NULL;
    bospectra_error_t err = bospectra_frame_acquire(64, 64, BOSPECTRA_PIXEL_FORMAT_ARGB32, &dummy_frame);
    if (err != BOSPECTRA_SUCCESS || !dummy_frame) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    if (sess->render_session_id != 0) {
        BOSPECTRA_Render_PresentFrame(sess->render_session_id, dummy_frame, 0, 0, 64, 64);
        sess->total_frames_rendered++;
    }

    bospectra_frame_release(dummy_frame);
    sess->total_frames_decoded++;
    return BOSPECTRA_SUCCESS;
}
