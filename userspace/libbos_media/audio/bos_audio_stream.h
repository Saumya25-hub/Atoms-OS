/*
 * ============================================================================
 * ATOMS OS — Userspace Audio Stream Interface
 * userspace/libbos_media/audio/bos_audio_stream.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Production Audio Stream Abstraction with Format Negotiation, Bounded FIFO,
 * and ATOMS Audio HAL (SYS_AUDIO_CALL) Integration.
 * ============================================================================
 */

#ifndef BOS_AUDIO_STREAM_H
#define BOS_AUDIO_STREAM_H

#include "kernel/audio/formats/audio_pcm.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOS_AUDIO_FIFO_CAPACITY (64 * 1024) // 64 KB bounded queue (~370ms PCM)

typedef struct BOSAudioStream {
    uint32_t       kernel_stream_id;
    AudioPcmFormat format;
    bool           is_playing;
    bool           is_configured;
    
    // Bounded FIFO Queue
    uint8_t        fifo_buffer[BOS_AUDIO_FIFO_CAPACITY];
    size_t         fifo_write_pos;
    size_t         fifo_read_pos;
    size_t         fifo_queued_bytes;

    // Telemetry & Metrics
    uint32_t       stat_underruns;
    uint32_t       stat_overruns;
    uint32_t       stat_backpressures;
    uint64_t       stat_total_bytes_written;
    uint64_t       stat_total_frames;
} BOSAudioStream;

BOSAudioStream* bos_audio_stream_create(void);
int             bos_audio_stream_configure(BOSAudioStream* s, uint32_t sample_rate, uint8_t channels, uint8_t bit_depth);
int             bos_audio_stream_start(BOSAudioStream* s);
int             bos_audio_stream_write(BOSAudioStream* s, const void* pcm_data, size_t size_bytes);
int             bos_audio_stream_write_fltp(BOSAudioStream* s, const float* const* channels, size_t num_samples, int num_channels);
int             bos_audio_stream_pause(BOSAudioStream* s);
int             bos_audio_stream_resume(BOSAudioStream* s);
int             bos_audio_stream_drain(BOSAudioStream* s);
int             bos_audio_stream_flush(BOSAudioStream* s);
int             bos_audio_stream_stop(BOSAudioStream* s);
void            bos_audio_stream_destroy(BOSAudioStream* s);

// Queue Metrics Accessors
size_t          bos_audio_stream_get_queued_bytes(BOSAudioStream* s);
size_t          bos_audio_stream_get_available_space(BOSAudioStream* s);

#ifdef __cplusplus
}
#endif

#endif /* BOS_AUDIO_STREAM_H */
