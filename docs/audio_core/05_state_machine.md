# Audio Core Foundation - State Machine

Every Audio Stream follows a strict, one-way (mostly) state machine. The Kernel Audio API enforces these transitions and silently rejects invalid requests, protecting the core from malicious or buggy applications.

## States
*   **CREATED**: The stream struct and ring buffer have been allocated in memory.
*   **READY**: The stream has been registered with the Audio Core and has a valid ID, but has not yet begun playback.
*   **PLAYING**: The stream is active. The mixer (future phase) will pull samples from this stream's ring buffer and mix them into the master output.
*   **PAUSED**: The stream is temporarily halted. The ring buffer remains intact and pointers are preserved, but the mixer ignores it.
*   **STOPPED**: The stream is halted, and its ring buffer is flushed (reset). Playback must begin from the start of the buffer.
*   **DESTROYED**: The stream has been unlinked from the core and all memory is freed. The ID is no longer valid.

## Valid Transitions
*   `CREATED -> READY`: Occurs internally during `audio_stream_create`.
*   `READY -> PLAYING`: Via `audio_stream_resume`.
*   `READY -> PAUSED`: Via `audio_stream_pause` (e.g. buffering data before starting).
*   `PLAYING -> PAUSED`: Via `audio_stream_pause`.
*   `PAUSED -> PLAYING`: Via `audio_stream_resume`.
*   `PLAYING/PAUSED -> STOPPED`: Via `audio_stream_stop`.
*   `ANY -> DESTROYED`: Via `audio_stream_destroy`.

Any attempt to pause a STOPPED stream, or play a DESTROYED stream, will be rejected by the API layer, returning `false`.
