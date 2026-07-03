# PCM Stream Engine - Format Definition

The ATOMS OS Audio Subsystem uses a universal descriptor to represent any raw PCM format.

## `AudioPcmFormat` Structure

```c
typedef struct {
    AudioPcmFormatType format;  // Enum: S8, U8, S16_LE, U16_LE
    uint32_t sample_rate;       // e.g., 44100, 48000, 22050
    uint8_t channels;           // 1 = Mono, 2 = Stereo
    uint8_t bit_depth;          // 8 or 16
    bool is_signed;             // true for signed, false for unsigned
} AudioPcmFormat;
```

## Supported Configurations

The engine currently supports and validates:
*   **Bit Depths**: 8-bit, 16-bit.
*   **Channels**: Mono (1), Stereo (2).
*   **Sample Rates**: Any valid positive integer, though standard rates (11025, 22050, 44100, 48000) are recommended.
*   **Signage**: Both Signed and Unsigned representations.

## Frame Calculations

A "Frame" represents one complete sample across all channels.
*   8-bit Mono = 1 Byte / Frame
*   8-bit Stereo = 2 Bytes / Frame
*   16-bit Mono = 2 Bytes / Frame
*   16-bit Stereo = 4 Bytes / Frame

The function `audio_pcm_bytes_per_frame()` computes this dynamically, ensuring that all data operations align perfectly to frame boundaries, preventing tearing.
