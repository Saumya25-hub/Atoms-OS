# PCM Stream Engine - Audio Packet

To stream data into the kernel, applications do not simply pass a raw byte pointer. They must encapsulate the data within an `AudioPcmPacket`.

## `AudioPcmPacket` Structure

```c
typedef struct {
    AudioPcmFormat format;
    uint32_t frame_count;
    uint64_t timestamp;
    uint32_t flags;
    const uint8_t* pcm_data;
    size_t size_bytes;
} AudioPcmPacket;
```

## Purpose

*   **Self-Describing**: The packet carries its own `AudioPcmFormat` metadata. When passed to `audio_stream_write()`, the kernel compares the packet's format against the target stream's format. If they do not match, the write is safely rejected.
*   **Integrity Checking**: By explicitly declaring both `frame_count` and `size_bytes`, the API can mathematically verify that the size aligns perfectly with the frame boundaries before ever touching memory.
*   **A/V Sync (Future)**: The `timestamp` field establishes a baseline for synchronizing video frames to audio playback in future media layers.

## Validation Routine
The API layer executes `audio_pcm_packet_is_valid()` on every write, asserting:
1. `pcm_data` is not NULL.
2. `size_bytes` matches `frame_count * audio_pcm_bytes_per_frame(format)`.
3. The embedded `format` is valid (supported bit depth and channel count).
