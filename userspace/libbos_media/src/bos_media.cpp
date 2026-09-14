/*
 * ============================================================================
 * ATOMS OS — Native Media Engine Implementation
 * userspace/libbos_media/src/bos_media.cpp
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements the stable BOS Media API wrapping the real BOSMediaPipeline.
 * Powered by genuine open-source engines:
 *   - Video: Hantro G1 H.264 + FFmpeg libavcodec CABAC
 *   - Demuxer: ISO MP4 Box Demuxer
 *   - Audio: minimp3 + dr_wav
 *   - Display: BOSurface v2.5
 *   - Sound: ATOMS Audio HAL
 * ============================================================================
 */

#include "../include/bos_media.h"
#include "bos_media_pipeline.h"
#include "userspace/runtime/c/include/atoms_syscall.h"
#include <string.h>

extern "C" void display_print(const char* s) {
    if (!s) return;
    size_t len = 0;
    while (s[len]) len++;
    if (len > 0) {
        __atoms_syscall2(SYS_WRITE, (uint64_t)s, (uint64_t)len);
    }
}

extern "C" void display_print_hex(uint64_t val) {
    char buf[17];
    for (int i = 15; i >= 0; i--) {
        buf[i] = "0123456789ABCDEF"[val & 0xF];
        val >>= 4;
    }
    buf[16] = '\0';
    display_print(buf);
}

extern "C" void bospectra_log(const char* tag, const char* msg) {
    (void)tag;
    if (msg) {
        display_print("[MEDIA] ");
        display_print(msg);
        display_print("\n");
    }
}

extern "C" uint32_t pci_get_device_count(void) {
    return 0;
}

extern "C" void* pci_get_device(uint32_t idx) {
    (void)idx;
    return nullptr;
}

struct BOSMediaPlayer {
    BOSMediaPipeline* pipeline;
};

static BOSMediaPlayer s_player __attribute__((aligned(16)));

BOSMediaPlayer* bos_media_create(void) {
    s_player.pipeline = bos_media_pipeline_create();
    return &s_player;
}

void bos_media_destroy(BOSMediaPlayer* player) {
    if (!player) return;
    if (player->pipeline) {
        bos_media_pipeline_destroy(player->pipeline);
        player->pipeline = nullptr;
    }
}

int bos_media_open(BOSMediaPlayer* player, const char* uri) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_open(player->pipeline, uri);
}

int bos_media_play(BOSMediaPlayer* player) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_play(player->pipeline);
}

int bos_media_pause(BOSMediaPlayer* player) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_pause(player->pipeline);
}

int bos_media_stop(BOSMediaPlayer* player) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_stop(player->pipeline);
}

int bos_media_seek(BOSMediaPlayer* player, int64_t position_ms) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_seek(player->pipeline, position_ms);
}

int bos_media_set_volume(BOSMediaPlayer* player, float volume) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_set_volume(player->pipeline, volume);
}

int bos_media_set_mute(BOSMediaPlayer* player, bool mute) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_set_mute(player->pipeline, mute);
}

int bos_media_set_video_surface(BOSMediaPlayer* player, void* surface_handle) {
    (void)player;
    (void)surface_handle;
    return BOS_MEDIA_OK;
}

int bos_media_render_frame(BOSMediaPlayer* player, uint32_t* target_fb, int target_w, int target_h, int stride_pixels) {
    if (!player || !player->pipeline) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_render_frame(player->pipeline, target_fb, target_w, target_h, stride_pixels);
}

BOSMediaState bos_media_get_state(BOSMediaPlayer* player) {
    if (!player || !player->pipeline) return BOS_MEDIA_STATE_IDLE;
    return bos_media_pipeline_get_state(player->pipeline);
}

int bos_media_get_position(BOSMediaPlayer* player, int64_t* out_position_ms) {
    if (!player || !player->pipeline || !out_position_ms) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_get_position(player->pipeline, out_position_ms);
}

int bos_media_get_duration(BOSMediaPlayer* player, int64_t* out_duration_ms) {
    if (!player || !player->pipeline || !out_duration_ms) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_get_duration(player->pipeline, out_duration_ms);
}

int bos_media_get_metadata(BOSMediaPlayer* player, BOSMediaMetadata* out_metadata) {
    if (!player || !player->pipeline || !out_metadata) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_get_metadata(player->pipeline, out_metadata);
}

int bos_media_get_telemetry(BOSMediaPlayer* player, BOSMediaTelemetry* out_telemetry) {
    if (!player || !player->pipeline || !out_telemetry) return BOS_MEDIA_ERROR_INVALID_PARAM;
    return bos_media_pipeline_get_telemetry(player->pipeline, out_telemetry);
}

void bos_media_tick(BOSMediaPlayer* player) {
    if (!player || !player->pipeline) return;
    bos_media_pipeline_tick(player->pipeline);
}
