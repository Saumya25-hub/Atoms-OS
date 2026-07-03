# PCM Stream Engine - Buffer Validation

To ensure kernel stability, the streaming layer assumes all incoming userspace data is potentially malicious or corrupted.

## Rejection Scenarios
`audio_stream_write` will silently reject data and return `0` bytes written under the following conditions:
1.  **Stale ID**: The `stream_id` points to a `DESTROYED` stream.
2.  **Null Pointers**: The packet pointer or `pcm_data` pointer is NULL.
3.  **Zero Length**: `size_bytes` or `frame_count` is 0.
4.  **Format Mismatch**: The packet's format (channels, bit depth, sample rate) does not exactly match the stream's configured format.
5.  **Math Corruption**: `frame_count * bytes_per_frame` does not equal `size_bytes`.

## Overflow and Underrun Handling
The ring buffer is strictly bounded.
*   **Overflow**: If an application attempts to write 4000 bytes into a buffer that only has 1000 bytes of free space, the engine will clamp the write. It writes exactly 1000 bytes, returns 1000, and increments the stream's `overflow_counter`. It does *not* overwrite unread data.
*   **Underrun**: If the mixer attempts to read 4000 bytes, but only 1000 are available, it will read 1000, return 1000, and increment the stream's `underrun_counter`.
