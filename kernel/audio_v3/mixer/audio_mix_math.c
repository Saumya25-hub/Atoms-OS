#include "kernel/audio/mixer/audio_mix_math.h"

int16_t audio_math_clamp16(int32_t sample) {
    if (sample > 32767) return 32767;
    if (sample < -32768) return -32768;
    return (int16_t)sample;
}

int8_t audio_math_clamp8(int32_t sample) {
    if (sample > 127) return 127;
    if (sample < -128) return -128;
    return (int8_t)sample;
}

uint8_t audio_math_clamp8u(int32_t sample) {
    if (sample > 255) return 255;
    if (sample < 0) return 0;
    return (uint8_t)sample;
}

void audio_math_mix_16(int32_t* accum, const int16_t* source, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        accum[i] += source[i];
    }
}

void audio_math_mix_8(int32_t* accum, const int8_t* source, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        accum[i] += source[i];
    }
}

void audio_math_mix_8u(int32_t* accum, const uint8_t* source, size_t samples) {
    for (size_t i = 0; i < samples; i++) {
        accum[i] += ((int32_t)source[i] - 128); // Convert unsigned to signed 8-bit equivalent
    }
}

void audio_math_normalize_16(int16_t* dest, const int32_t* accum, size_t samples, uint32_t* clipped_count, int32_t* peak_amplitude) {
    uint32_t clips = 0;
    int32_t peak = 0;
    
    for (size_t i = 0; i < samples; i++) {
        int32_t val = accum[i];
        
        // Track peak
        int32_t abs_val = val;
        if (abs_val < 0) abs_val = -abs_val;
        if (abs_val > peak) peak = abs_val;
        
        // Clamp and copy
        if (val > 32767) {
            dest[i] = 32767;
            clips++;
        } else if (val < -32768) {
            dest[i] = -32768;
            clips++;
        } else {
            dest[i] = (int16_t)val;
        }
    }
    
    if (clipped_count) *clipped_count += clips;
    if (peak_amplitude && peak > *peak_amplitude) *peak_amplitude = peak;
}
