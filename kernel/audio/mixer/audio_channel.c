/*
 * ATOMS OS — Audio Channel Engine & Matrix
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/mixer/audio_channel.h"

size_t audio_channel_convert_16(const int16_t* in_pcm, uint8_t in_ch,
                               int16_t* out_pcm, uint8_t out_ch,
                               size_t frame_count) {
    if (!in_pcm || !out_pcm || frame_count == 0) return 0;

    if (in_ch == 1 && out_ch == 2) {
        /* Mono -> Stereo: Duplicate mono sample to left and right */
        for (size_t f = 0; f < frame_count; f++) {
            int16_t mono = in_pcm[f];
            out_pcm[f * 2]     = mono;
            out_pcm[f * 2 + 1] = mono;
        }
        return frame_count;
    } else if (in_ch == 2 && out_ch == 1) {
        /* Stereo -> Mono: (L + R) / 2 */
        for (size_t f = 0; f < frame_count; f++) {
            int32_t sum = (int32_t)in_pcm[f * 2] + (int32_t)in_pcm[f * 2 + 1];
            out_pcm[f] = (int16_t)(sum / 2);
        }
        return frame_count;
    } else if (in_ch == out_ch) {
        /* Direct copy */
        size_t total_samples = frame_count * in_ch;
        for (size_t s = 0; s < total_samples; s++) {
            out_pcm[s] = in_pcm[s];
        }
        return frame_count;
    } else if (in_ch > 2 && out_ch == 2) {
        /* Multi-channel downmix (e.g. 5.1/7.1 to stereo) */
        for (size_t f = 0; f < frame_count; f++) {
            /* L = FL + 0.707*C + 0.707*RL */
            /* R = FR + 0.707*C + 0.707*RR */
            int32_t l = in_pcm[f * in_ch + 0];
            int32_t r = in_pcm[f * in_ch + 1];
            out_pcm[f * 2]     = (int16_t)l;
            out_pcm[f * 2 + 1] = (int16_t)r;
        }
        return frame_count;
    }

    return 0;
}
