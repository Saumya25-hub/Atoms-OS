#include "playback_session.h"
#include "../../memory/bospectra_memory.h"
#include "../../file/bospectra_file.h"
#include "../../container/registry/container_registry.h"
#include "../../container/avi/avi_parser.h"
#include "../../container/mp4/mp4_parser.h"
#include "../../container/mkv/mkv_parser.h"
#include "../../decoder/mjpeg/mjpeg_decoder.h"
#include "../../decoder/h264/h264_decoder.h"
#include "../../decoder/mpeg2/mpeg2_decoder.h"
#include "../../color/include/bospectra_color.h"
#include "../../frame_memory/packet_pool/packet_pool.h"
#include "../../frame_memory/include/bospectra_frame.h"
#include "../../manager/container_manager.h"
#include "../../manager/decoder_manager.h"
#include "../../manager/render_manager.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_PLAYBACK_SESSIONS 4U

static PlaybackSessionCtx g_playback_sessions[BOSPECTRA_MAX_PLAYBACK_SESSIONS];
static bool g_session_subsystem_initialized = false;

void playback_session_subsystem_init(void) {
    memset(g_playback_sessions, 0, sizeof(g_playback_sessions));
    g_session_subsystem_initialized = true;
}

void playback_session_subsystem_shutdown(void) {
    for (uint32_t i = 0; i < BOSPECTRA_MAX_PLAYBACK_SESSIONS; i++) {
        if (g_playback_sessions[i].is_active) {
            playback_session_destroy(g_playback_sessions[i].session_id);
        }
    }
    memset(g_playback_sessions, 0, sizeof(g_playback_sessions));
    g_session_subsystem_initialized = false;
}

PlaybackSessionCtx* playback_session_get_by_id(bospectra_playback_session_id_t id) {
    if (!g_session_subsystem_initialized || id == 0 || id > BOSPECTRA_MAX_PLAYBACK_SESSIONS) return NULL;
    uint32_t idx = id - 1;
    return g_playback_sessions[idx].is_active ? &g_playback_sessions[idx] : NULL;
}

bospectra_error_t playback_session_set_render_target(bospectra_playback_session_id_t id, uint32_t window_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    sess->target_window_id = window_id;
    sess->target_x = x;
    sess->target_y = y;
    sess->target_w = w;
    sess->target_h = h;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t playback_session_create(const char* media_uri, bospectra_playback_session_id_t* out_session_id) {
    if (!g_session_subsystem_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!media_uri || !out_session_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint32_t slot = BOSPECTRA_MAX_PLAYBACK_SESSIONS;
    for (uint32_t i = 0; i < BOSPECTRA_MAX_PLAYBACK_SESSIONS; i++) {
        if (!g_playback_sessions[i].is_active) {
            slot = i;
            break;
        }
    }
    if (slot >= BOSPECTRA_MAX_PLAYBACK_SESSIONS) return BOSPECTRA_ERR_BUFFER_OVERFLOW;

    PlaybackSessionCtx* sess = &g_playback_sessions[slot];
    memset(sess, 0, sizeof(PlaybackSessionCtx));
    sess->session_id = slot + 1;
    strncpy(sess->media_uri, media_uri, sizeof(sess->media_uri) - 1);

    playback_state_init(&sess->state_machine);
    playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_OPENING);
    bospectra_trace_str("TRACE 15 — Playback State Change", "OPENING");

    /* Open Media File via BOSPECTRA File Subsystem */
    bospectra_error_t err = bospectra_file_open(media_uri, &sess->file_id);
    if (err == BOSPECTRA_SUCCESS) {
        /* Dynamically Probe Container Engine Driver via Container Manager */
        const BOSPECTRA_ContainerDriver* drv = bospectra_container_auto_probe(sess->file_id);
        if (!drv) {
            drv = &g_avi_container_driver; /* Safety Fallback */
        }
        sess->container_driver = drv;
        BOSPECTRA_ContainerMetadata meta;
        memset(&meta, 0, sizeof(meta));

        err = sess->container_driver->open(&sess->container_ctx, sess->file_id, &meta);
        if (err == BOSPECTRA_SUCCESS) {
            /* Query video stream descriptor */
            BOSPECTRA_StreamDescriptor stream_desc;
            memset(&stream_desc, 0, sizeof(stream_desc));
            if (sess->container_driver->get_stream(sess->container_ctx, 0, &stream_desc) == BOSPECTRA_SUCCESS) {
                /* Dynamically Route Video Decoder Driver via Decoder Manager */
                sess->decoder_driver = bospectra_decoder_resolve(&stream_desc);
                if (sess->decoder_driver && sess->decoder_driver->open) {
                    sess->decoder_driver->open(&sess->decoder_ctx, &stream_desc);
                }
            }

            uint64_t dur = meta.duration_us ? meta.duration_us : 60000000ULL;
            playback_timeline_init(&sess->timeline, dur);
        }
    }

    if (!sess->timeline.duration_us) {
        playback_timeline_init(&sess->timeline, 60000000ULL);
    }
    playback_events_init(&sess->event_bus);

    /* Open Audio Subsystem Session */
    BOSPECTRA_AudioSpec audio_spec;
    audio_spec.format = BOSPECTRA_AUDIO_FORMAT_PCM_S16LE;
    audio_spec.sample_rate = 44100;
    audio_spec.channels = 2;
    audio_spec.bits_per_sample = 16;
    BOSPECTRA_Audio_OpenSession(&audio_spec, &sess->audio_session_id);

    /* Open Render Subsystem Session */
    BOSPECTRA_RenderSpec render_spec;
    render_spec.backend = BOSPECTRA_RENDER_BACKEND_SOFTWARE;
    render_spec.filter = BOSPECTRA_SCALING_FILTER_BILINEAR;
    render_spec.rotation = BOSPECTRA_ROTATION_0;
    render_spec.target_window_id = 0; /* Will be updated via playback_session_set_render_target */
    render_spec.flags = BOSPECTRA_RENDER_FLAG_VSYNC;
    BOSPECTRA_Render_OpenSession(&render_spec, &sess->render_session_id);

    /* OPENING → READY → PAUSED (OPENING→PAUSED is invalid per state machine) */
    playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_READY);
    bospectra_trace_str("TRACE 15 — Playback State Change", "READY");
    playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_PAUSED);
    bospectra_trace_str("TRACE 15 — Playback State Change", "PAUSED");

    /* Initialize Staged Pipeline Engine & Frame Scheduler */
    bospectra_pipeline_context_init(&sess->pipeline_ctx);
    bospectra_frame_scheduler_init(&sess->frame_scheduler_ctx, 30);
    bospectra_master_clock_start(&sess->frame_scheduler_ctx.master_clock, 0);

    sess->is_active = true;
    *out_session_id = sess->session_id;

    bospectra_trace_str("TRACE 6 — Playback Session Created", "SUCCESS");
    bospectra_trace_u32("Playback Session ID", sess->session_id);
    bospectra_trace_str("State", "PAUSED");
    bospectra_trace_u32("Timeline Position US", (uint32_t)sess->timeline.current_position_us);
    bospectra_trace_u32("Duration US", (uint32_t)sess->timeline.duration_us);
    bospectra_trace_u32("Render Session ID", sess->render_session_id);

    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t pipeline_stage_demux(void* ctx) {
    PlaybackSessionCtx* sess = (PlaybackSessionCtx*)ctx;
    BOSPacket* pkt = NULL;
    bospectra_error_t err = sess->container_driver->read_packet(sess->container_ctx, &pkt);
    if (err == BOSPECTRA_SUCCESS && pkt) {
        err = bospectra_packet_queue_enqueue(&sess->pipeline_ctx.demux_packet_queue, pkt);
        if (err != BOSPECTRA_SUCCESS) {
            bospectra_packet_pool_release(pkt);
        }
    } else if (err == BOSPECTRA_ERR_BUFFER_UNDERFLOW) {
        sess->container_driver->seek(sess->container_ctx, 0);
    }
    return err;
}

static bospectra_error_t pipeline_stage_decode(void* ctx) {
    PlaybackSessionCtx* sess = (PlaybackSessionCtx*)ctx;
    BOSPacket* pkt = NULL;
    if (bospectra_packet_queue_dequeue(&sess->pipeline_ctx.demux_packet_queue, &pkt) != BOSPECTRA_SUCCESS || !pkt) {
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    BOSFrame* decoded_frame = NULL;
    bospectra_error_t err = sess->decoder_driver->decode_packet(sess->decoder_ctx, pkt, &decoded_frame);
    bospectra_packet_pool_release(pkt);

    if (err == BOSPECTRA_SUCCESS && decoded_frame) {
        sess->total_frames_decoded++;
        err = bospectra_frame_queue_enqueue(&sess->pipeline_ctx.decoded_frame_queue, decoded_frame);
        if (err != BOSPECTRA_SUCCESS) {
            bospectra_frame_release(decoded_frame);
        }
    }
    return err;
}

#include "../../scheduler/display_scheduler.h"

static bospectra_error_t pipeline_stage_render(void* ctx) {
    PlaybackSessionCtx* sess = (PlaybackSessionCtx*)ctx;
    BOSFrame* decoded_frame = NULL;
    if (bospectra_frame_queue_peek(&sess->pipeline_ctx.decoded_frame_queue, &decoded_frame) != BOSPECTRA_SUCCESS || !decoded_frame) {
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    BOSPECTRA_FrameAction action = bospectra_frame_scheduler_evaluate(&sess->frame_scheduler_ctx, decoded_frame->pts);

    if (action == FRAME_ACTION_TOO_EARLY) {
        /* Hold frame in queue until presentation time arrives */
        return BOSPECTRA_SUCCESS;
    }

    /* Pop frame from queue */
    (void)bospectra_frame_queue_dequeue(&sess->pipeline_ctx.decoded_frame_queue, &decoded_frame);

    if (action == FRAME_ACTION_LATE_DROP) {
        /* Drop late frame to catch up */
        bospectra_frame_release(decoded_frame);
        return BOSPECTRA_SUCCESS;
    }

    bospectra_display_scheduler_present(sess->render_session_id, decoded_frame,
                                       (int32_t)sess->target_x, (int32_t)sess->target_y,
                                       (int32_t)sess->target_w, (int32_t)sess->target_h);
    sess->total_frames_rendered++;
    playback_timeline_update_position(&sess->timeline, decoded_frame->pts);
    bospectra_frame_release(decoded_frame);

    return BOSPECTRA_SUCCESS;
}

void playback_session_tick(bospectra_playback_session_id_t id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(id);
    if (!sess || !sess->is_active) return;

    /* Run pipeline in both PLAYING and PAUSED state to unblock initial frame render */
    bospectra_playback_state_t st = sess->state_machine.current_state;
    if (st != BOSPECTRA_PLAYBACK_STATE_PLAYING && st != BOSPECTRA_PLAYBACK_STATE_PAUSED) return;
    if (!sess->container_driver || !sess->container_ctx || !sess->decoder_driver || !sess->decoder_ctx) return;

    /* Update Master Clock using frame pacer native microsecond interval (Step 3 Bug 1 Fix) */
    uint64_t frame_duration_us = sess->frame_scheduler_ctx.pacer.frame_interval_us;
    if (frame_duration_us == 0) frame_duration_us = 41666U; /* Default 24 FPS fallback (41.6ms) */
    bospectra_master_clock_update(&sess->frame_scheduler_ctx.master_clock, frame_duration_us);

    bospectra_pipeline_scheduler_step(&sess->pipeline_ctx.scheduler, sess,
                                      pipeline_stage_demux,
                                      pipeline_stage_decode,
                                      pipeline_stage_render);

    /* Auto-transition to PLAYING state after first successful pipeline step */
    if (st == BOSPECTRA_PLAYBACK_STATE_PAUSED && sess->total_frames_decoded > 0) {
        playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_PLAYING);
    }
}

bospectra_error_t playback_session_destroy(bospectra_playback_session_id_t id) {
    PlaybackSessionCtx* sess = playback_session_get_by_id(id);
    if (!sess) return BOSPECTRA_ERR_HANDLE_INVALID;

    playback_state_transition(&sess->state_machine, BOSPECTRA_PLAYBACK_STATE_STOPPED);
    playback_events_post(&sess->event_bus, BOSPECTRA_EVENT_MEDIA_CLOSED, id);

    if (sess->decoder_driver && sess->decoder_ctx) {
        sess->decoder_driver->close(sess->decoder_ctx);
        sess->decoder_ctx = NULL;
    }

    if (sess->container_driver && sess->container_ctx) {
        sess->container_driver->close(sess->container_ctx);
        sess->container_ctx = NULL;
    }

    if (sess->file_id != 0) {
        bospectra_file_close(sess->file_id);
        sess->file_id = 0;
    }

    if (sess->audio_session_id != 0) {
        BOSPECTRA_Audio_CloseSession(sess->audio_session_id);
        sess->audio_session_id = 0;
    }

    if (sess->render_session_id != 0) {
        BOSPECTRA_Render_CloseSession(sess->render_session_id);
        sess->render_session_id = 0;
    }

    memset(sess, 0, sizeof(PlaybackSessionCtx));
    return BOSPECTRA_SUCCESS;
}
