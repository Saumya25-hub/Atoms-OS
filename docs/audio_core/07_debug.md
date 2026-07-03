# Audio Core Foundation - Debug & Telemetry

The `audio_debug.c` module provides real-time visibility into the memory profile of the audio subsystem. Since audio streams are created and destroyed rapidly (e.g. UI clicks), identifying memory leaks is critical.

## Metrics Tracked
*   **Active Streams**: Current number of allocated streams.
*   **Peak Streams**: High-water mark for simultaneous streams.
*   **Destroyed Streams**: Cumulative total of streams torn down.
*   **Allocated Buffers**: Current number of active allocations (`kmalloc` calls).
*   **Allocated Bytes**: Current total bytes held by the subsystem.
*   **Peak Memory**: High-water mark for memory consumption.
*   **Failed Allocations**: Tracks if the kernel heap rejected an audio allocation.

## Usage
The telemetry is automatically updated via wrapper functions (`audio_debug_log_alloc`, `audio_debug_log_free`) called directly by the allocation logic in `audio_stream.c` and `audio_buffer.c`.

A snapshot of the telemetry can be dumped to the screen at any time by calling `audio_debug_print_stats()`.
