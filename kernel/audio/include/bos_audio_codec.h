/*
 * ATOMS OS — Universal Audio Codec Interface
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef BOS_AUDIO_CODEC_H
#define BOS_AUDIO_CODEC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kernel/audio/formats/audio_pcm.h"

#define BOS_AUDIO_METADATA_MAX_STR 128

typedef struct {
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bit_depth;
    uint64_t total_samples;
    uint32_t duration_ms;
    uint32_t bitrate_kbps;
    char     title[BOS_AUDIO_METADATA_MAX_STR];
    char     artist[BOS_AUDIO_METADATA_MAX_STR];
    char     album[BOS_AUDIO_METADATA_MAX_STR];
    char     codec_name[32];
} bos_audio_info_t;

typedef struct bos_audio_codec_handle {
    void* internal_state;
    bos_audio_info_t info;
    const struct bos_audio_codec_driver* driver;
} bos_audio_codec_handle_t;

typedef struct bos_audio_codec_driver {
    const char* name;
    bool (*probe)(const uint8_t* data, size_t size, const char* filepath);
    bos_audio_codec_handle_t* (*open)(const uint8_t* data, size_t size);
    bool (*get_info)(bos_audio_codec_handle_t* handle, bos_audio_info_t* out_info);
    size_t (*decode)(bos_audio_codec_handle_t* handle, int16_t* out_pcm, size_t max_samples);
    bool (*seek)(bos_audio_codec_handle_t* handle, uint64_t target_sample);
    void (*close)(bos_audio_codec_handle_t* handle);
} bos_audio_codec_driver_t;

#endif /* BOS_AUDIO_CODEC_H */
