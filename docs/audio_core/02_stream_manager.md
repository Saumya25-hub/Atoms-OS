# Audio Core Foundation - Stream Manager

## The `AudioStream` Structure
At the heart of the subsystem is the `AudioStream` struct, representing a single active sound session (e.g., a background music track, a UI click, or a game sound effect).

### Fields
*   `stream_id`: A globally unique identifier for this session.
*   `process_id`: The ID of the application that created this stream.
*   `state`: The current position in the Audio State Machine.
*   `volume`: 0-255 scaling factor.
*   `sample_rate`: e.g., 44100.
*   `channels`: Mono (1) or Stereo (2).
*   `format`: Bit depth (e.g., 16).
*   `ring_buffer`: Pointer to the exclusive `AudioRingBuffer` owned by this stream.
*   `ref_count`: Tracks active references to prevent use-after-free bugs.
*   `timestamp`: Playback timing for future A/V sync.
*   `driver_handle`: (Future) Pointer to the hardware driver this stream is routed to.

## Lifecycle Management
The Stream Manager (`audio_core.c`) maintains a singly-linked list of all active `AudioStream` objects.

*   **Registration**: When an app requests a stream, the core allocates a new ID, instantiates the struct (which in turn allocates the ring buffer), and pushes it to the head of the global list.
*   **Lookup**: The core provides `audio_core_get_stream()` to safely retrieve a stream by ID.
*   **Destruction**: When a stream is destroyed, it is unlinked from the list, its ring buffer is freed, and the struct is deallocated. The operation returns a boolean indicating success, preventing double-frees.
