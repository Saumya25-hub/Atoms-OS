#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <stdint.h>
#include <stddef.h>
#include "audio_pcm.h"

void audio_mixer_init(void);
void audio_mixer_shutdown(void);
void audio_mixer_reset(void);

bool audio_mixer_add_stream(uint32_t stream_id);
bool audio_mixer_remove_stream(uint32_t stream_id);

// Mixes data from all registered streams into the output buffer
// Returns the number of bytes successfully mixed (usually max_bytes)
size_t audio_mixer_process(uint8_t* output_buffer, size_t max_bytes, const AudioPcmFormat* target_format);

void audio_mixer_get_stats(uint32_t* mixed_streams, uint64_t* frames_mixed, uint64_t* clipped_samples, int32_t* peak_amplitude);

#endif // AUDIO_MIXER_H
