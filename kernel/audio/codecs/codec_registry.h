/*
 * ATOMS OS — BOS Audio Codec Registry
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef BOS_CODEC_REGISTRY_H
#define BOS_CODEC_REGISTRY_H

#include "kernel/audio/include/bos_audio_codec.h"

void codec_registry_init(void);

const bos_audio_codec_driver_t* codec_registry_probe(const uint8_t* data, size_t size, const char* filepath);

bos_audio_codec_handle_t* codec_registry_open(const uint8_t* data, size_t size, const char* filepath);

#endif /* BOS_CODEC_REGISTRY_H */
