/*
 * ATOMS OS — Audio Channel Engine & Matrix
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * Converts 16-bit PCM across channel layouts.
 * Supports:
 * - 1 -> 2 (Mono to Stereo)
 * - 2 -> 1 (Stereo to Mono)
 * - 2 -> 2 (Identity copy)
 * Returns number of output frames written.
 */
size_t audio_channel_convert_16(const int16_t* in_pcm, uint8_t in_ch,
                               int16_t* out_pcm, uint8_t out_ch,
                               size_t frame_count);

#endif /* AUDIO_CHANNEL_H */
