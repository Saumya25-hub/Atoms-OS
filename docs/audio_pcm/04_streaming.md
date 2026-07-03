# PCM Stream Engine - Streaming API

Data enters and exits the audio subsystem through the `audio_api.c` boundaries.

## Endpoints

*   **`audio_stream_write(stream_id, packet)`**: 
    The primary userspace entry point. Extracts `pcm_data` from the packet and pushes it into the stream's lock-free ring buffer. Returns the exact number of bytes successfully written.
*   **`audio_stream_read(stream_id, buffer, size)`**: 
    The primary kernel/mixer entry point (Phase 7.3+). Pulls PCM data out of the ring buffer to be mixed or sent to hardware.
*   **`audio_stream_available(stream_id)`**: 
    Returns the number of unread bytes currently waiting in the ring buffer.
*   **`audio_stream_capacity(stream_id)`**: 
    Returns the total byte capacity of the stream's ring buffer (typically 16KB).
*   **`audio_stream_flush(stream_id)`** & **`audio_stream_reset(stream_id)`**: 
    Immediately clears all unread data from the ring buffer, bringing the available byte count to 0. Useful for skipping tracks or pausing playback without stuttering old data.
