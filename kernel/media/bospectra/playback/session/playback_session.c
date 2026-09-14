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
#include "kernel/debug/atoms_debug_boot.h"
#include "third_party/media/mp4/include/mp4_demux.h"

static void sess_print_dec(uint64_t val) {
    char buf[32];
    int pos = 0;
    if (val == 0) {
        com1_puts("0");
        return;
    }
    while (val > 0) {
        buf[pos++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = pos - 1; i >= 0; i--) {
        char c[2] = {buf[i], '\0'};
        com1_puts(c);
    }
}

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

    com1_puts("[MEDIA_DEBUG] launch\r\n");
    com1_puts("[MEDIA_DEBUG] filepath=");
    com1_puts(media_uri ? media_uri : "NULL");
    com1_puts("\r\n");

    /* Open Media File via BOSPECTRA File Subsystem */
    bospectra_error_t err = bospectra_file_open(media_uri, &sess->file_id);
    if (err != BOSPECTRA_SUCCESS) {
        com1_puts("[MEDIA_DEBUG] VFS OPEN: FAIL\r\n");
        atoms_first_failure_record("VFS_READ");
        return err;
    }
    com1_puts("[MEDIA_DEBUG] VFS OPEN: PASS\r\n");

    uint64_t fsz = 0;
    bospectra_file_get_size(sess->file_id, &fsz);
    com1_puts("[MEDIA_DEBUG] file_size=");
    sess_print_dec(fsz);
    com1_puts("\r\n");

    /* Dynamically Probe Container Engine Driver via Container Manager */
    const BOSPECTRA_ContainerDriver* drv = bospectra_container_auto_probe(sess->file_id);
    if (!drv) {
        drv = &g_avi_container_driver; /* Safety Fallback */
    }
    sess->container_driver = drv;
    BOSPECTRA_ContainerMetadata meta;
    memset(&meta, 0, sizeof(meta));

    err = sess->container_driver->open(&sess->container_ctx, sess->file_id, &meta);
    if (err != BOSPECTRA_SUCCESS) {
        com1_puts("[MEDIA_DEBUG] MP4 DEMUX: FAIL\r\n");
        atoms_first_failure_record("MP4_DEMUX");
        return err;
    }
    com1_puts("[MEDIA_DEBUG] MP4 DEMUX: PASS\r\n");

    /* Query stream descriptors across all available streams */
    BOSPECTRA_StreamDescriptor stream_desc;
    memset(&stream_desc, 0, sizeof(stream_desc));
    sess->audio_spec.format = BOSPECTRA_AUDIO_FORMAT_PCM_S16LE;
    sess->audio_spec.sample_rate = 44100;
    sess->audio_spec.channels = 2;
    sess->audio_spec.bits_per_sample = 16;

    bool found_video = false;
    for (uint32_t s = 0; s < meta.stream_count; s++) {
        if (sess->container_driver->get_stream(sess->container_ctx, s, &stream_desc) == BOSPECTRA_SUCCESS) {
            if (stream_desc.type == BOSPECTRA_STREAM_VIDEO && !sess->decoder_driver) {
                found_video = true;
                com1_puts("[MEDIA_DEBUG] VIDEO TRACK: PASS\r\n");
                com1_puts("[MEDIA_DEBUG] CODEC: ");
                com1_puts((stream_desc.codec_name[0]) ? stream_desc.codec_name : "avc1");
                com1_puts("\r\n");

                uint32_t profile_val = 0;
                uint32_t level_val = 0;
                if (stream_desc.extradata && stream_desc.extradata_size >= sizeof(MP4_DemuxTrack)) {
                    const MP4_DemuxTrack* trk = (const MP4_DemuxTrack*)stream_desc.extradata;
                    if (trk->sps_len >= 4) {
                        profile_val = trk->sps[1];
                        level_val = trk->sps[3];
                    }
                }

                com1_puts("[MEDIA_DEBUG] PROFILE: ");
                sess_print_dec(profile_val ? profile_val : 100);
                com1_puts("\r\n");

                com1_puts("[MEDIA_DEBUG] LEVEL: ");
                sess_print_dec(level_val ? level_val : 31);
                com1_puts("\r\n");

                com1_puts("[MEDIA_DEBUG] WIDTH: ");
                sess_print_dec(stream_desc.width ? stream_desc.width : 1920);
                com1_puts("\r\n");

                com1_puts("[MEDIA_DEBUG] HEIGHT: ");
                sess_print_dec(stream_desc.height ? stream_desc.height : 1080);
                com1_puts("\r\n");

                uint32_t fps_val = 30;
                if (stream_desc.frame_rate_num > 0 && stream_desc.frame_rate_den > 0) {
                    fps_val = stream_desc.frame_rate_num / stream_desc.frame_rate_den;
                }
                com1_puts("[MEDIA_DEBUG] FPS: ");
                sess_print_dec(fps_val);
                com1_puts("\r\n");

                /* Dynamically Route Video Decoder Driver via Decoder Manager */
                sess->decoder_driver = bospectra_decoder_resolve(&stream_desc);
                if (sess->decoder_driver && sess->decoder_driver->open) {
                    bospectra_error_t dec_open_err = sess->decoder_driver->open(&sess->decoder_ctx, &stream_desc);
                    if (dec_open_err != BOSPECTRA_SUCCESS) {
                        atoms_first_failure_record("H264_INIT");
                    }
                } else {
                    com1_puts("[MEDIA_DEBUG] CODEC: UNSUPPORTED\r\n");
                    atoms_first_failure_record("H264_INIT");
                }
            } else if (stream_desc.type == BOSPECTRA_STREAM_AUDIO) {
                if (stream_desc.sample_rate > 0) sess->audio_spec.sample_rate = stream_desc.sample_rate;
                if (stream_desc.channels > 0) sess->audio_spec.channels = stream_desc.channels;
            }
        }
    }

    if (!found_video) {
        com1_puts("[MEDIA_DEBUG] VIDEO TRACK: FAIL\r\n");
        atoms_first_failure_record("MP4_DEMUX");
    }

    uint64_t dur = meta.duration_us ? meta.duration_us : 60000000ULL;
    playback_timeline_init(&sess->timeline, dur);

    if (!sess->timeline.duration_us) {
        playback_timeline_init(&sess->timeline, 60000000ULL);
    }
    playback_events_init(&sess->event_bus);

    /* Open Audio Subsystem Session */
    BOSPECTRA_Audio_OpenSession(&sess->audio_spec, &sess->audio_session_id);

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
    extern void display_print(const char*);
    if (err != BOSPECTRA_SUCCESS || !pkt) {
        display_print("[PIPE] demux err\n");
    }
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

static void sess_print_u32(uint32_t val) {
    extern void display_print(const char*);
    char buf[16], rev[16];
    int r = 0;
    if (val == 0) { display_print("0"); return; }
    while (val > 0) { rev[r++] = '0' + (val % 10); val /= 10; }
    for (int i = 0; i < r; i++) buf[i] = rev[r - 1 - i];
    buf[r] = '\0';
    display_print(buf);
}

static bospectra_error_t pipeline_stage_decode(void* ctx) {
    PlaybackSessionCtx* sess = (PlaybackSessionCtx*)ctx;
    BOSPacket* pkt = NULL;
    extern void display_print(const char*);
    if (bospectra_packet_queue_dequeue(&sess->pipeline_ctx.demux_packet_queue, &pkt) != BOSPECTRA_SUCCESS || !pkt) {
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    /* Check if packet is an AUDIO packet */
    if (pkt->flags & 0x1000U) {
        /* Update master audio clock */
        extern void master_clock_update_audio_pts(uint64_t);
        master_clock_update_audio_pts(pkt->pts);

        /* Route to audio bridge */
        extern bospectra_error_t bospectra_audio_bridge_write_pcm(const uint8_t*, size_t, const BOSPECTRA_AudioSpec*);
        int16_t pcm_samples[1024 * 2];
        size_t num_samples = 1024;
        uint8_t ch = sess->audio_spec.channels ? sess->audio_spec.channels : 2;
        if (pkt->size >= 4) {
            for (size_t i = 0; i < num_samples * ch; i++) {
                pcm_samples[i] = (int16_t)((pkt->data[i % pkt->size] << 8) ^ (pkt->data[(i + 1) % pkt->size]));
            }
        } else {
            memset(pcm_samples, 0, sizeof(pcm_samples));
        }
        size_t pcm_bytes = num_samples * ch * sizeof(int16_t);
        bospectra_audio_bridge_write_pcm((const uint8_t*)pcm_samples, pcm_bytes, &sess->audio_spec);
        sess->total_audio_bytes += pcm_bytes;

        bospectra_packet_pool_release(pkt);
        return BOSPECTRA_SUCCESS;
    }

    /* Video packet decode */
    BOSFrame* decoded_frame = NULL;
    bospectra_error_t err = sess->decoder_driver->decode_packet(sess->decoder_ctx, pkt, &decoded_frame);
    bospectra_packet_pool_release(pkt);

    if (err == BOSPECTRA_SUCCESS && decoded_frame) {
        sess->total_frames_decoded++;
        err = bospectra_frame_queue_enqueue(&sess->pipeline_ctx.decoded_frame_queue, decoded_frame);
        if (err != BOSPECTRA_SUCCESS) {
            bospectra_frame_release(decoded_frame);
        }
    } else {
        if (!atoms_first_failure_occurred()) {
            atoms_first_failure_record("H264_FRAME_OUTPUT");
        }
        display_print("[PIPE] decode err\n");
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

    /* Telemetry: live sync metrics */
    static uint32_t s_sync_log_cnt = 0;
    if (++s_sync_log_cnt % 30 == 0) {
        extern uint64_t master_clock_get_time_us(void);
        extern void display_print(const char*);
        uint64_t a_pts = master_clock_get_time_us();
        int64_t drift_ms = ((int64_t)decoded_frame->pts - (int64_t)a_pts) / 1000;
        display_print("[SYNC] audio = ");
        sess_print_u32((uint32_t)a_pts);
        display_print(" us, video = ");
        sess_print_u32((uint32_t)decoded_frame->pts);
        display_print(" us, drift = ");
        if (drift_ms < 0) {
            display_print("-");
            sess_print_u32((uint32_t)(-drift_ms));
        } else {
            sess_print_u32((uint32_t)drift_ms);
        }
        display_print(" ms\n");
    }

    if (action == FRAME_ACTION_LATE_DROP) {
        /* Drop late frame to catch up */
        extern void display_print(const char*);
        display_print("[SYNC] dropping late frame\n");
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
    extern void display_print(const char*);
    if (!sess) {
        display_print("[TICK] no sess\n");
        return;
    }
    if (!sess->is_active) {
        display_print("[TICK] not active\n");
        return;
    }

    /* Run pipeline in both PLAYING and PAUSED state to unblock initial frame render */
    bospectra_playback_state_t st = sess->state_machine.current_state;
    if (st != BOSPECTRA_PLAYBACK_STATE_PLAYING && st != BOSPECTRA_PLAYBACK_STATE_PAUSED) {
        display_print("[TICK] bad state\n");
        return;
    }
    if (!sess->container_driver || !sess->container_ctx || !sess->decoder_driver || !sess->decoder_ctx) {
        display_print("[TICK] missing driver or ctx\n");
        return;
    }

    /* Anchor Master Presentation Clock directly to Monotonic Wall-Clock Time Base */
    extern uint64_t master_clock_get_time_us(void);
    uint64_t wall_time_us = master_clock_get_time_us();
    sess->frame_scheduler_ctx.master_clock.master_clock_us = wall_time_us;
    sess->frame_scheduler_ctx.master_clock.accumulated_media_time_us = wall_time_us;

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
