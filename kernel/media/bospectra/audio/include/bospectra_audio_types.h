#ifndef BOSPECTRA_AUDIO_TYPES_H
#define BOSPECTRA_AUDIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BOSPECTRA_AUDIO_FORMAT_UNKNOWN = 0,
    BOSPECTRA_AUDIO_FORMAT_PCM_S16LE,   // Signed 16-bit Little Endian (Default)
    BOSPECTRA_AUDIO_FORMAT_PCM_U8,      // Unsigned 8-bit PCM
    BOSPECTRA_AUDIO_FORMAT_PCM_F32LE   // 32-bit Float PCM (Future Ready)
} bospectra_audio_format_t;

typedef enum {
    BOSPECTRA_AUDIO_CHANNEL_MONO = 1,
    BOSPECTRA_AUDIO_CHANNEL_STEREO = 2
} bospectra_audio_channels_t;

typedef uint32_t bospectra_audio_session_id_t;

typedef struct {
    bospectra_audio_format_t format;
    uint32_t                 sample_rate; // 44100 Hz, 48000 Hz, etc.
    uint8_t                  channels;    // 1 = Mono, 2 = Stereo
    uint8_t                  bits_per_sample; // 8 or 16
} BOSPECTRA_AudioSpec;

#endif // BOSPECTRA_AUDIO_TYPES_H
