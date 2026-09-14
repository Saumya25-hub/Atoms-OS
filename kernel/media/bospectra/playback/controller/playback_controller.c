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

    /* 1. Flush Demux Packet Queue */
    BOSPacket* pkt = NULL;
    while (bospectra_packet_queue_dequeue(&sess->pipeline_ctx.demux_packet_queue, &pkt) == BOSPECTRA_SUCCESS && pkt) {
        bospectra_packet_pool_release(pkt);
    }

    /* 2. Flush Decoded Frame Queue */
    BOSFrame* frame = NULL;
    while (bospectra_frame_queue_dequeue(&sess->pipeline_ctx.decoded_frame_queue, &frame) == BOSPECTRA_SUCCESS && frame) {
        bospectra_frame_release(frame);
    }

    /* 3. Container Demuxer Seek */
    if (sess->container_driver && sess->container_ctx && sess->container_driver->seek) {
        sess->container_driver->seek(sess->container_ctx, target_position_us);
    }

    /* 4. Decoder Driver Flush (clear DPB) */
    if (sess->decoder_driver && sess->decoder_ctx && sess->decoder_driver->flush) {
        sess->decoder_driver->flush(sess->decoder_ctx);
    }

    /* 5. Audio Flush */
    if (sess->audio_session_id != 0) {
        BOSPECTRA_Audio_Flush(sess->audio_session_id);
    }

    /* 6. Master Clock & Timeline Seek */
    playback_timeline_seek(&sess->timeline, target_position_us);
    BOSPECTRA_Sync_UpdateAudioClock(target_position_us);
    bospectra_master_clock_seek(&sess->frame_scheduler_ctx.master_clock, target_position_us);

    /* 7. Resume previous playback state */
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

    if (!sess->container_driver || !sess->container_ctx || !sess->decoder_driver || !sess->decoder_ctx) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    /* First check if a frame is already queued in decoded_frame_queue */
    BOSFrame* decoded_frame = NULL;
    if (bospectra_frame_queue_dequeue(&sess->pipeline_ctx.decoded_frame_queue, &decoded_frame) == BOSPECTRA_SUCCESS && decoded_frame) {
        if (sess->render_session_id != 0) {
            BOSPECTRA_Render_PresentFrame(sess->render_session_id, decoded_frame,
                                          sess->target_x, sess->target_y, sess->target_w, sess->target_h);
            sess->total_frames_rendered++;
        }
        playback_timeline_update_position(&sess->timeline, decoded_frame->pts);
        bospectra_frame_release(decoded_frame);
        return BOSPECTRA_SUCCESS;
    }

    /* Otherwise, decode from container until exactly one video frame is produced */
    uint32_t attempts = 0;
    while (attempts++ < 50) {
        BOSPacket* pkt = NULL;
        bospectra_error_t err = sess->container_driver->read_packet(sess->container_ctx, &pkt);
        if (err != BOSPECTRA_SUCCESS || !pkt) break;

        /* Skip audio packet in single-frame video step */
        if (pkt->flags & 0x1000U) {
            bospectra_packet_pool_release(pkt);
            continue;
        }

        err = sess->decoder_driver->decode_packet(sess->decoder_ctx, pkt, &decoded_frame);
        bospectra_packet_pool_release(pkt);

        if (err == BOSPECTRA_SUCCESS && decoded_frame) {
            sess->total_frames_decoded++;
            if (sess->render_session_id != 0) {
                BOSPECTRA_Render_PresentFrame(sess->render_session_id, decoded_frame,
                                              sess->target_x, sess->target_y, sess->target_w, sess->target_h);
                sess->total_frames_rendered++;
            }
            playback_timeline_update_position(&sess->timeline, decoded_frame->pts);
            bospectra_frame_release(decoded_frame);
            return BOSPECTRA_SUCCESS;
        }
    }

    return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
}
