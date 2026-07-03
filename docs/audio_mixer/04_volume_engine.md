# Software Mixer Engine - Volume Engine

The `audio_volume` layer utilizes linear attenuation through integer scalar math.

## Global State
*   **Master Volume**: A master scalar `0-255` applied across all mixed streams.
*   **Mute State**: A global boolean override.

## Multiplication Scaling
Since floating-point math (e.g., `vol = 0.5f`) is strictly prohibited, the engine uses integer scaling logic:

`final_amplitude = (amplitude * stream_volume * master_volume) / (255 * 255)`

Because `stream_volume` and `master_volume` max out at `255`, multiplying them yields a maximum scalar of `65025`. The engine multiplies the 16-bit PCM amplitude (which is cast to 32-bit temporary space) by this scalar, and then mathematically divides by `65025` (`255 * 255`) to bring it back to its relative original scale.

This ensures that at `255/255`, the output is `100%`, and at `127/255` the output is roughly `50%`.

## Short-Circuit Optimization
To preserve CPU cycles on heavily loaded systems, the volume engine implements short-circuit checks:
*   If Mute is active, or volume is `0`, `memset(0)` is used instantly.
*   If volume is `255`, the engine entirely skips the scaling loop and returns the untouched buffer.
