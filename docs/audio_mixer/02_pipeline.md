# Software Mixer Engine - Pipeline

The standard processing pipeline executes dynamically upon the call to `audio_mixer_process()`.

## Workflow
1.  **Target Assessment**: The caller provides an `output_buffer`, a `max_bytes` length, and the `target_format` required by the final hardware layer.
2.  **Accumulator Clearing**: A 32-bit integer array (`accum_buffer`) is zeroed out for the duration of the mix.
3.  **Stream Traversal**: The mixer asks `audio_core_get_active_streams()` for the active stream list.
4.  **Per-Stream Fetch**: For each active stream:
    *   The mixer requests up to `max_bytes` of data via `audio_stream_read()`.
    *   If data is available, it is pulled into a `temp_buffer`.
5.  **Per-Stream Volume**: The `audio_volume_apply_16()` algorithm multiplies the stream data by both its internal volume and the master volume.
6.  **Summation**: `audio_math_mix_16()` iterates through the volume-adjusted data, adding it to the `accum_buffer`.
7.  **Final Normalization**: Once all streams are processed, `audio_math_normalize_16()` iterates over the `accum_buffer`, clamping any values that exceed `INT16_MAX` or `INT16_MIN` safely into the `output_buffer`.
8.  **Telemetry Reporting**: Clipped samples, active stream count, and peak amplitudes are recorded for debug logging.
