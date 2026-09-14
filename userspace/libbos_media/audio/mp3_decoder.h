/*
 * ============================================================================
 * ATOMS OS — Userspace MP3 Audio Decoder Wrapper
 * userspace/libbos_media/audio/mp3_decoder.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Powered by minimp3 (CC0 1.0 Universal / Public Domain)
 * ============================================================================
 */

#ifndef MP3_DECODER_H
#define MP3_DECODER_H

#include "../include/bos_media_stream.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BOSMp3Decoder BOSMp3Decoder;

BOSMp3Decoder* bos_mp3_decoder_create(BOSMediaStream* stream);
void           bos_mp3_decoder_destroy(BOSMp3Decoder* dec);

/* Decodes the next frame into out_pcm (must be at least 2304 int16_t samples) */
int            bos_mp3_decoder_read_frame(BOSMp3Decoder* dec, int16_t* out_pcm, int max_samples,
                                          int* out_samples, int* out_channels, int* out_hz);

int            bos_mp3_decoder_get_info(BOSMp3Decoder* dec, int* out_channels, int* out_hz, int* out_bitrate_kbps);

#ifdef __cplusplus
}
#endif

#endif /* MP3_DECODER_H */
