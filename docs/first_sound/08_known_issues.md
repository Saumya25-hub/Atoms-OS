# First Sound - Known Issues

## 1. Blocking Initialization Loop
Currently, the test runs as a single blocking `while` loop within `audio_test_tone_run()`. It prevents the scheduler from transitioning to the next boot phase until the target frames are met.
- **Fix (Phase 7.7+)**: Move the playback generator into a background kernel thread (or user-space application) utilizing hardware interrupts to signal when the buffer needs refilling.

## 2. Hardcoded Values
The sample rate (48000Hz) and bit depth (16-bit stereo) are hardcoded into the generator loop.
- **Fix (Phase 7.x)**: Implement dynamic format negotiation and resampling within the Software Mixer to handle non-native formats natively.
