/*
 * ATOMS OS — Fixed-Point Audio Resampler Engine
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/mixer/audio_resampler.h"
#ifdef BOS_HOST_TEST
#include <string.h>
#else
#include "kernel/core/lib/include/string.h"
#endif

void audio_resampler_init(audio_resampler_t* r, uint32_t in_rate, uint32_t out_rate, uint8_t channels) {
    if (!r) return;
    r->in_rate = (in_rate > 0) ? in_rate : 48000;
    r->out_rate = (out_rate > 0) ? out_rate : 48000;
    r->channels = (channels > 0) ? channels : 2;
    r->frac_pos = 0;
    r->last_sample[0] = 0;
    r->last_sample[1] = 0;
}

void audio_resampler_reset(audio_resampler_t* r) {
    if (!r) return;
    r->frac_pos = 0;
    r->last_sample[0] = 0;
    r->last_sample[1] = 0;
}

size_t audio_resample_linear_16(audio_resampler_t* r,
                               const int16_t* in_pcm, size_t in_frames,
                               int16_t* out_pcm, size_t max_out_frames,
                               size_t* in_frames_consumed) {
    if (!r || !in_pcm || !out_pcm || in_frames == 0 || max_out_frames == 0) {
        if (in_frames_consumed) *in_frames_consumed = 0;
        return 0;
    }

    /* Pass-through if identical rate */
    if (r->in_rate == r->out_rate) {
        size_t to_copy = (in_frames < max_out_frames) ? in_frames : max_out_frames;
        size_t samples = to_copy * r->channels;
        for (size_t s = 0; s < samples; s++) {
            out_pcm[s] = in_pcm[s];
        }
        if (to_copy > 0) {
            r->last_sample[0] = in_pcm[(to_copy - 1) * r->channels];
            if (r->channels > 1) {
                r->last_sample[1] = in_pcm[(to_copy - 1) * r->channels + 1];
            }
        }
        if (in_frames_consumed) *in_frames_consumed = to_copy;
        return to_copy;
    }

    /* 16.16 fixed-point step: in_rate / out_rate */
    uint64_t step_fp = ((uint64_t)r->in_rate << 16) / r->out_rate;
    uint32_t pos = r->frac_pos;
    size_t out_f = 0;
    uint8_t ch = r->channels;

    while (out_f < max_out_frames) {
        uint32_t in_idx = pos >> 16;
        uint32_t frac = pos & 0xFFFF;

        if (in_idx >= in_frames) {
            break;
        }

        for (uint8_t c = 0; c < ch; c++) {
            int32_t s0, s1;
            if (in_idx == 0 && frac == 0) {
                s0 = in_pcm[c];
                s1 = (in_frames > 1) ? in_pcm[ch + c] : s0;
            } else {
                s0 = in_pcm[in_idx * ch + c];
                s1 = (in_idx + 1 < in_frames) ? in_pcm[(in_idx + 1) * ch + c] : s0;
            }

            int32_t interp = (s0 * (int32_t)(65536 - frac) + s1 * (int32_t)frac) >> 16;
            if (interp > 32767) interp = 32767;
            if (interp < -32768) interp = -32768;
            out_pcm[out_f * ch + c] = (int16_t)interp;
        }

        out_f++;
        pos += (uint32_t)step_fp;
    }

    /* Calculate consumed frames */
    uint32_t consumed = pos >> 16;
    if (consumed > in_frames) {
        consumed = (uint32_t)in_frames;
    }
    r->frac_pos = pos & 0xFFFF;

    if (consumed > 0) {
        r->last_sample[0] = in_pcm[(consumed - 1) * ch];
        if (ch > 1) {
            r->last_sample[1] = in_pcm[(consumed - 1) * ch + 1];
        }
    }

    if (in_frames_consumed) {
        *in_frames_consumed = consumed;
    }
    return out_f;
}
