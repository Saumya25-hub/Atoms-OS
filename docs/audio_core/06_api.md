# Audio Core Foundation - API

The `audio_api.c` module provides the high-level boundary for the Audio Subsystem.

## Core Lifecycle
*   `void audio_init(void)`: Initializes the core and debug telemetry. Called exactly once during kernel boot.
*   `void audio_shutdown(void)`: Walks the global stream list and destroys all active streams, freeing all memory.

## Stream Management
*   `uint32_t audio_stream_create(uint32_t process_id)`: Allocates a new stream. Returns a unique ID, or 0 if allocation failed.
*   `bool audio_stream_destroy(uint32_t stream_id)`: Destroys the stream. Returns `true` if successful, or `false` if the ID was invalid (preventing double-free panics).

## Playback Control
*   `bool audio_stream_pause(uint32_t stream_id)`: Transitions a `PLAYING` or `READY` stream to `PAUSED`.
*   `bool audio_stream_resume(uint32_t stream_id)`: Transitions a `PAUSED` or `READY` stream to `PLAYING`.
*   `bool audio_stream_stop(uint32_t stream_id)`: Transitions to `STOPPED` and flushes the ring buffer.
*   `bool audio_set_volume(uint32_t stream_id, uint8_t volume)`: Sets the software volume (0-255).

## Telemetry
*   `void audio_get_stats(void)`: Prints current memory usage and stream statistics to the kernel display.
