# PCM Stream Engine - Synthetic Testing

To stress test the PCM data flow without a hardware dependency, the engine includes a suite of synthetic waveform generators in `audio_pcm.c`.

## Supported Waveforms
All generators utilize purely integer math (no floating point) to remain kernel-safe. They support both 8-bit/16-bit and Signed/Unsigned dynamically based on the provided `AudioPcmFormat`.
*   **Sine Wave**: Generated via a static 256-point lookup table and phase accumulator. Used for pure frequency testing (e.g. 440 Hz, 880 Hz).
*   **Square Wave**: Evaluates the phase accumulator MSB to alternate between maximum and minimum amplitude.
*   **Saw Wave**: Directly maps the linear phase accumulator to amplitude.
*   **White Noise**: Utilizes a basic Linear Congruential Generator (LCG) PRNG to fill the buffer with static.
*   **Silence**: Fills the buffer with algorithmic zero (0x00 for signed, 0x80 for 8-bit unsigned).

## The Test Suite
During kernel boot, `audio_debug_test_pcm_engine()` executes. It configures a virtual stream, generates a 440 Hz Sine wave payload into an `AudioPcmPacket`, and fires it into the `audio_stream_write` API, kicking off the integrity validations.
