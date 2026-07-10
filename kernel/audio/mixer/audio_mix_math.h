#ifndef AUDIO_MIX_MATH_H
#define AUDIO_MIX_MATH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

int16_t audio_math_clamp16(int32_t sample);
int8_t audio_math_clamp8(int32_t sample);
uint8_t audio_math_clamp8u(int32_t sample);

void audio_math_mix_16(int32_t* accum, const int16_t* source, size_t samples);
void audio_math_mix_8(int32_t* accum, const int8_t* source, size_t samples);
void audio_math_mix_8u(int32_t* accum, const uint8_t* source, size_t samples);

void audio_math_normalize_16(int16_t* dest, const int32_t* accum, size_t samples, uint32_t* clipped_count, int32_t* peak_amplitude);

#endif // AUDIO_MIX_MATH_H
