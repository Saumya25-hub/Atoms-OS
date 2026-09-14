/*
 * ============================================================================
 * ATOMS OS — BOS libmpv Adapter Internal Interface
 * userspace/libbos_media/mpv/mpv_adapter.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Connects libmpv embedding APIs to ATOMS VFS, Audio HAL, and BOSurface
 * ============================================================================
 */

#ifndef MPV_ADAPTER_H
#define MPV_ADAPTER_H

#include "../include/bos_media.h"
#include "third_party/media/mpv/include/mpv/client.h"
#include "third_party/media/mpv/include/mpv/render.h"
#include "third_party/media/mpv/include/mpv/stream_cb.h"
#include "userspace/runtime/c/include/atoms_syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BOSMpvAdapter BOSMpvAdapter;

struct BOSMpvAdapter {
    mpv_handle*         mpv;
    mpv_render_context* render_ctx;
    
    BOSMediaState       state;
    BOSMediaMetadata    meta;
    BOSMediaTelemetry   telemetry;

    /* VFS Stream State */
    char                current_uri[256];
    int                 vfs_fd;
    int64_t             file_size;

    /* Audio HAL State */
    uint32_t            audio_stream_id;
    uint32_t            audio_sample_rate;
    uint32_t            audio_channels;
    uint32_t            audio_samples_written;
    float               volume;
    bool                is_muted;

    /* Video Presentation State */
    void*               surface_handle;
    uint32_t*           video_frame_buffer;
    int                 video_width;
    int                 video_height;
    uint64_t            last_frame_pts;
    uint64_t            start_time_ms;
    
    /* Diagnostics */
    bool                frame_ready;
    uint32_t            frame_crc;
};

/* Adapter Lifecycle */
BOSMpvAdapter*  mpv_adapter_create(void);
void            mpv_adapter_destroy(BOSMpvAdapter* adapter);

/* Playback & Navigation */
int             mpv_adapter_open(BOSMpvAdapter* adapter, const char* uri);
int             mpv_adapter_play(BOSMpvAdapter* adapter);
int             mpv_adapter_pause(BOSMpvAdapter* adapter);
int             mpv_adapter_stop(BOSMpvAdapter* adapter);
int             mpv_adapter_seek(BOSMpvAdapter* adapter, int64_t position_ms);
int             mpv_adapter_set_volume(BOSMpvAdapter* adapter, float volume);
int             mpv_adapter_set_mute(BOSMpvAdapter* adapter, bool mute);

/* Video Render & Surface Blit */
int             mpv_adapter_set_video_surface(BOSMpvAdapter* adapter, void* surface_handle);
int             mpv_adapter_render_frame(BOSMpvAdapter* adapter, uint32_t* target_fb, int target_w, int target_h, int stride_pixels);

/* Event & Stream Pump */
void            mpv_adapter_tick(BOSMpvAdapter* adapter);

/* Stream Callback (VFS Bridge) */
int             mpv_vfs_stream_open(void* user_data, char* uri, mpv_stream_cb_info* info);

/* Audio HAL Output Bridge */
void            mpv_audio_hal_init(BOSMpvAdapter* adapter);
void            mpv_audio_hal_shutdown(BOSMpvAdapter* adapter);
void            mpv_audio_hal_write_pcm(BOSMpvAdapter* adapter, const void* data, size_t bytes, uint32_t samples);

#ifdef __cplusplus
}
#endif

#endif /* MPV_ADAPTER_H */
