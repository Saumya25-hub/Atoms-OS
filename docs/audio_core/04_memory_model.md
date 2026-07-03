# Audio Core Foundation - Memory Model

## Zero-Leak Strategy

The Audio Core is built with strict memory ownership rules. There are no shared ownership semantics; everything is strictly hierarchical.

### Hierarchy
1.  **Process**: Owns an array of Stream IDs.
2.  **Audio Core**: Owns the global linked list of `AudioStream` objects.
3.  **Audio Stream**: Owns exactly one `AudioRingBuffer`.
4.  **Audio Buffer**: Owns one dynamically allocated byte array (`data`).

### Deallocation Chain
When `audio_stream_destroy(id)` is called:
1.  The Stream is removed from the global list.
2.  `audio_stream_destroy_obj` is called.
3.  This automatically calls `audio_buffer_destroy`.
4.  Which automatically calls `kfree(data)` followed by `kfree(buffer)`.
5.  Finally, `kfree(stream)` is called.

### Safety Guarantees
*   **No Orphans**: A Ring Buffer cannot exist without an Audio Stream. If the stream creation fails, the buffer is never allocated (or immediately freed).
*   **Process Teardown (Future)**: When the OS process manager destroys a dying process, it will simply query the Audio Core for all streams matching `process_id` and destroy them. This guarantees no audio buffers leak when an application crashes.
*   **Telemetry Backed**: Every allocation and free is tracked by `audio_debug.c`. The self-test routine explicitly verifies that memory usage returns exactly to 0 after stress-testing stream creation/destruction.
