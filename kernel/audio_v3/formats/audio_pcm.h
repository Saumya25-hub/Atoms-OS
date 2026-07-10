#ifndef AUDIO_PCM_H
#define AUDIO_PCM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PCM_FORMAT_S8,
    PCM_FORMAT_U8,
    PCM_FORMAT_S16_LE,
    PCM_FORMAT_U16_LE
} AudioPcmFormatType;

typedef struct {
    AudioPcmFormatType format;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bit_depth;
    bool is_signed;
} AudioPcmFormat;

typedef struct {
    AudioPcmFormat format;
    uint32_t frame_count;
    uint64_t timestamp;
    uint32_t flags;
    const uint8_t* pcm_data;
    size_t size_bytes;
} AudioPcmPacket;

size_t audio_pcm_bytes_per_frame(const AudioPcmFormat* format);
bool audio_pcm_format_is_valid(const AudioPcmFormat* format);
bool audio_pcm_packet_is_valid(const AudioPcmPacket* packet);

// Synthetic Generators
void audio_pcm_generate_silence(const AudioPcmFormat* format, uint8_t* buffer, size_t frames);
void audio_pcm_generate_sine(const AudioPcmFormat* format, uint32_t freq, uint8_t* buffer, size_t frames, uint32_t* phase_accum);
void audio_pcm_generate_noise(const AudioPcmFormat* format, uint8_t* buffer, size_t frames, uint32_t* seed);
void audio_pcm_generate_square(const AudioPcmFormat* format, uint32_t freq, uint8_t* buffer, size_t frames, uint32_t* phase_accum);
void audio_pcm_generate_saw(const AudioPcmFormat* format, uint32_t freq, uint8_t* buffer, size_t frames, uint32_t* phase_accum);

// Forensic verification
void audio_pcm_generate_stereo_sine(const AudioPcmFormat* format, uint32_t freq_l, uint32_t freq_r, uint8_t* buffer, size_t frames, uint32_t* phase_l, uint32_t* phase_r);

typedef struct {
    uint32_t frequency;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits;
    uint32_t total_samples;
    int32_t peak_pos;
    int32_t peak_neg;
    uint32_t rms;
    int32_t dc_offset;
    uint32_t zero_crossings;
    uint32_t checksum;
} AudioPcmStats;

void audio_math_calculate_stats(const AudioPcmFormat* format, const uint8_t* buffer, size_t frames, uint32_t freq, AudioPcmStats* out_stats);

#endif // AUDIO_PCM_H
