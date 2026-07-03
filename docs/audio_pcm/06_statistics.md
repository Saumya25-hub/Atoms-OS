# PCM Stream Engine - Statistics & Telemetry

Every `AudioStream` natively tracks its own performance metrics via the nested `AudioStreamStats` structure.

## Tracked Metrics
*   `bytes_written` / `frames_written`: Cumulative data successfully pushed into the buffer.
*   `bytes_read` / `frames_read`: Cumulative data successfully pulled by the mixer.
*   `overflow_counter`: Number of times an application attempted to write more data than the buffer could hold.
*   `underrun_counter`: Number of times the mixer starved (attempted to read when the buffer was empty).
*   `flush_counter` / `reset_counter`: Number of times the buffer was manually cleared.

## Global Aggregation
When `audio_debug_print_stats()` is invoked, the telemetry engine iterates over the global stream list, aggregates the statistics across all *currently active* streams, and displays the totals on the kernel console.

```text
PCM Bytes Written (Active): 256000
PCM Bytes Read (Active): 256000
PCM Underruns (Active): 0
PCM Overflows (Active): 0
```
This data is crucial for profiling audio latency and stability when hardware drivers are introduced.
