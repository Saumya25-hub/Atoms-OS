/*
 * ATOMS OS — Fixed-Point Audio Resampler Engine
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef AUDIO_RESAMPLER_H
#define AUDIO_RESAMPLER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint32_t in_rate;
    uint32_t out_rate;
    uint8_t  channels;
    uint32_t frac_pos; /* 16.16 fixed point phase accumulator */
    int16_t  last_sample[2]; /* Previous boundary sample per channel */
} audio_resampler_t;

void audio_resampler_init(audio_resampler_t* r, uint32_t in_rate, uint32_t out_rate, uint8_t channels);
void audio_resampler_reset(audio_resampler_t* r);

/*
 * Resamples 16-bit interleaved PCM from in_pcm to out_pcm.
 * Returns the number of output frames produced.
 * *in_frames_consumed returns how many input frames were read.
 */
size_t audio_resample_linear_16(audio_resampler_t* r,
                               const int16_t* in_pcm, size_t in_frames,
                               int16_t* out_pcm, size_t max_out_frames,
                               size_t* in_frames_consumed);

#endif /* AUDIO_RESAMPLER_H */
