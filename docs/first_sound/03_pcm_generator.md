# First Sound - PCM Generator

To avoid complex decoders (like WAV or MP3), the validation tone is generated using pure integer math.

## Generator Specs
- **Frequency**: 440Hz (A4 pitch).
- **Sample Rate**: 48,000 Hz.
- **Bit Depth**: 16-bit.
- **Channels**: 2 (Stereo).
- **Math**: Integer-only to prevent floating-point context switching issues in the kernel.

The `phase_accum` variable is preserved across generation chunks to ensure the sine wave remains perfectly continuous without clicking or popping at chunk boundaries.
