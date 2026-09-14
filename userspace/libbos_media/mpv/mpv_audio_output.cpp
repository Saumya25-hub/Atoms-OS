/*
 * ============================================================================
 * ATOMS OS — BOS libmpv Audio HAL Output Bridge
 * userspace/libbos_media/mpv/mpv_audio_output.cpp
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Directs decoded PCM audio (48kHz S16_LE Stereo) to SYS_AUDIO_CALL
 * for native DMA output via Intel HDA / AC97 hardware.
 * ============================================================================
 */

#include "mpv_adapter.h"
#include "kernel/audio/formats/audio_pcm.h"
#include <string.h>

extern "C" void display_print(const char* s);

static void audio_print(const char* s) {
    if (!s) return;
    display_print(s);
}

void mpv_audio_hal_init(BOSMpvAdapter* adapter) {
    if (!adapter) return;

    // Default target format: 48,000 Hz, 16-bit Signed Little-Endian Stereo
    adapter->audio_sample_rate = 48000;
    adapter->audio_channels = 2;
    adapter->audio_samples_written = 0;
    adapter->volume = 1.0f;
    adapter->is_muted = false;

    uint64_t sid = __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_CREATE, 0);
    if (sid != (uint64_t)-1 && sid != 0) {
        adapter->audio_stream_id = (uint32_t)sid;
        audio_print("[MEDIA-P1] AUDIO_STREAM_CREATE: PASS\n");

        AudioPcmFormat fmt;
        fmt.format = PCM_FORMAT_S16_LE;
        fmt.sample_rate = adapter->audio_sample_rate;
        fmt.channels = (uint8_t)adapter->audio_channels;
        fmt.bit_depth = 16;
        fmt.is_signed = true;

        __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_SET_FORMAT, adapter->audio_stream_id, (uint64_t)&fmt);
        __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_START, adapter->audio_stream_id);
        __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_DEVICE_SET_VOL, adapter->audio_stream_id, 255);
    }
}

void mpv_audio_hal_shutdown(BOSMpvAdapter* adapter) {
    if (!adapter || adapter->audio_stream_id == 0) return;

    __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_STOP, adapter->audio_stream_id);
    __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_DESTROY, adapter->audio_stream_id);
    adapter->audio_stream_id = 0;
    audio_print("[MEDIA-P1] AUDIO_STREAM_DESTROY: PASS\n");
}

void mpv_audio_hal_write_pcm(BOSMpvAdapter* adapter, const void* data, size_t bytes, uint32_t samples) {
    if (!adapter || !data || bytes == 0 || adapter->audio_stream_id == 0) return;

    adapter->audio_samples_written += samples;
    uint64_t pts_ms = (adapter->audio_sample_rate > 0) ? 
        ((uint64_t)adapter->audio_samples_written * 1000ULL / adapter->audio_sample_rate) : 0;
    adapter->telemetry.audio_pts_ms = (int64_t)pts_ms;

    // Calculate A/V drift in milliseconds
    if (adapter->telemetry.video_pts_ms > 0) {
        int64_t diff = adapter->telemetry.audio_pts_ms - adapter->telemetry.video_pts_ms;
        adapter->telemetry.drift_ms = (diff >= 0) ? diff : -diff;
    }

    AudioPcmPacket pkt;
    pkt.format.format = PCM_FORMAT_S16_LE;
    pkt.format.sample_rate = adapter->audio_sample_rate;
    pkt.format.channels = (uint8_t)adapter->audio_channels;
    pkt.format.bit_depth = 16;
    pkt.format.is_signed = true;
    pkt.frame_count = samples;
    pkt.timestamp = pts_ms;
    pkt.flags = 0;
    pkt.pcm_data = (const uint8_t*)data;
    pkt.size_bytes = bytes;

    __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_WRITE, adapter->audio_stream_id, (uint64_t)&pkt);
}
