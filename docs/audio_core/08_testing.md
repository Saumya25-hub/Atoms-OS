# Audio Core Foundation - Self Testing

To guarantee the stability of the foundation before hardware drivers are introduced, the subsystem includes a built-in software self-test: `audio_debug_run_selftest()`.

## Test Execution
The self-test is executed immediately after `audio_init()` during kernel boot.

## Test Phases
1.  **Single Stream Lifecycle**: Creates and destroys a stream, validating API return values.
2.  **Invalid Destruction**: Attempts to destroy the stream a second time. Validates that the API rejects the double-free safely without crashing.
3.  **Stress Test (100 Streams)**: Creates 100 streams simultaneously, simulating a heavy load. Then destroys all 100 streams.
4.  **Ring Buffer Logic**: 
    *   Writes a sequence of bytes.
    *   Reads the bytes back and validates the payload matches perfectly.
    *   Attempts to over-fill the buffer and validates the write logic correctly clamps the input without overflowing the memory boundary.
5.  **Leak Detection**: Finally, it asserts that `telemetry.allocated_bytes`, `telemetry.allocated_buffers`, and `telemetry.active_streams` are exactly 0. If any are >0, a leak is detected and the test fails.

## Results
The self-test successfully passes all phases on boot, printing:
`[AUDIO SELF-TEST] SUCCESS: All tests passed with zero leaks.`
