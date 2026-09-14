/*
 * ============================================================================
 * ATOMS OS — Native Media Engine Public API
 * userspace/libbos_media/include/bos_media.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Public LGPLv2.1+ Compliant Media Abstraction Layer
 * Isolates ATOMS applications from decoder & demuxer internals.
 * ============================================================================
 */

#ifndef BOS_MEDIA_H
#define BOS_MEDIA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Error Codes
 * ============================================================================
 */
typedef enum {
    BOS_MEDIA_OK                    = 0,
    BOS_MEDIA_ERROR_GENERIC         = -1,
    BOS_MEDIA_ERROR_INVALID_PARAM   = -2,
    BOS_MEDIA_ERROR_OUT_OF_MEMORY   = -3,
    BOS_MEDIA_ERROR_FILE_NOT_FOUND  = -4,
    BOS_MEDIA_ERROR_ACCESS_DENIED   = -5,
    BOS_MEDIA_ERROR_INVALID_CONTAINER = -6,
    BOS_MEDIA_ERROR_UNSUPPORTED_CODEC = -7,
    BOS_MEDIA_ERROR_DECODER_INIT    = -8,
    BOS_MEDIA_ERROR_AUDIO_INIT      = -9,
    BOS_MEDIA_ERROR_VIDEO_RENDER    = -10,
    BOS_MEDIA_ERROR_NOT_INITIALIZED = -11,
    BOS_MEDIA_ERROR_DEVICE_BUSY     = -12
} BOSMediaError;

/* ============================================================================
 * Playback State
 * ============================================================================
 */
typedef enum {
    BOS_MEDIA_STATE_IDLE       = 0,
    BOS_MEDIA_STATE_OPENING    = 1,
    BOS_MEDIA_STATE_BUFFERING  = 2,
    BOS_MEDIA_STATE_PLAYING    = 3,
    BOS_MEDIA_STATE_PAUSED     = 4,
    BOS_MEDIA_STATE_STOPPED    = 5,
    BOS_MEDIA_STATE_ERROR      = 6
} BOSMediaState;

/* ============================================================================
 * Media Metadata
 * ============================================================================
 */
typedef struct {
    char    title[128];
    char    artist[128];
    char    album[128];
    char    container[32];
    char    video_codec[32];
    char    audio_codec[32];
    int64_t duration_ms;
    int32_t video_width;
    int32_t video_height;
    double  video_fps;
    int32_t audio_sample_rate;
    int32_t audio_channels;
    bool    has_video;
    bool    has_audio;
} BOSMediaMetadata;

/* ============================================================================
 * Synchronization & Performance Telemetry
 * ============================================================================
 */
typedef struct {
    int64_t  audio_pts_ms;
    int64_t  video_pts_ms;
    int64_t  drift_ms;
    uint32_t decoded_frames;
    uint32_t presented_frames;
    uint32_t dropped_frames;
    float    cpu_percent;
    uint32_t memory_kb;
    const char* first_failure_stage;
    const char* first_failure_reason;
    bool     is_hardware_accelerated;
    const char* acceleration_backend;
} BOSMediaTelemetry;

/* Opaque player handle */
typedef struct BOSMediaPlayer BOSMediaPlayer;

/* ============================================================================
 * Core Lifecycle APIs
 * ============================================================================
 */
BOSMediaPlayer* bos_media_create(void);
void            bos_media_destroy(BOSMediaPlayer* player);

/* Playback Controls */
int             bos_media_open(BOSMediaPlayer* player, const char* uri);
int             bos_media_play(BOSMediaPlayer* player);
int             bos_media_pause(BOSMediaPlayer* player);
int             bos_media_stop(BOSMediaPlayer* player);
int             bos_media_seek(BOSMediaPlayer* player, int64_t position_ms);
int             bos_media_set_volume(BOSMediaPlayer* player, float volume); /* 0.0f - 1.0f */
int             bos_media_set_mute(BOSMediaPlayer* player, bool mute);

/* Surface & Output Integration */
int             bos_media_set_video_surface(BOSMediaPlayer* player, void* surface_handle);
int             bos_media_render_frame(BOSMediaPlayer* player, uint32_t* target_fb, int target_w, int target_h, int stride_pixels);

/* State & Diagnostics */
BOSMediaState   bos_media_get_state(BOSMediaPlayer* player);
int             bos_media_get_position(BOSMediaPlayer* player, int64_t* out_position_ms);
int             bos_media_get_duration(BOSMediaPlayer* player, int64_t* out_duration_ms);
int             bos_media_get_metadata(BOSMediaPlayer* player, BOSMediaMetadata* out_metadata);
int             bos_media_get_telemetry(BOSMediaPlayer* player, BOSMediaTelemetry* out_telemetry);

/* Non-blocking main loop pump */
void            bos_media_tick(BOSMediaPlayer* player);

/* Telemetry dump functions */
void            bos_media_dump_sync_telemetry(BOSMediaPlayer* player);
void            bos_media_dump_perf_telemetry(BOSMediaPlayer* player, uint64_t startup_ms, uint64_t first_frame_ms, double fps);
void            bos_media_report_stage(const char* stage, const char* status, const char* details);

#ifdef __cplusplus
}
#endif

#endif /* BOS_MEDIA_H */
