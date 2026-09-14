/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Audio Output Adapter (Chromium media::AudioOutputStream)
 */

#ifndef ATOMS_APAL_AUDIO_H
#define ATOMS_APAL_AUDIO_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t sample_rate;    /* 44100 or 48000 Hz */
    uint16_t channels;       /* 1 (Mono) or 2 (Stereo) */
    uint16_t bits_per_sample;/* 16-bit PCM */
    uint32_t buffer_frames;
} apal_audio_config_t;

typedef apal_handle_t apal_audio_stream_t;

apal_status_t apal_audio_stream_open(const apal_audio_config_t *config, apal_audio_stream_t *out_stream);
int64_t apal_audio_stream_write(apal_audio_stream_t stream, const void *pcm_data, size_t bytes);
apal_status_t apal_audio_stream_flush(apal_audio_stream_t stream);
apal_status_t apal_audio_stream_close(apal_audio_stream_t stream);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_AUDIO_H */
