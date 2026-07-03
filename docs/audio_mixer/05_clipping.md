# Software Mixer Engine - Clipping Protection

Clipping occurs when the arithmetic sum of multiple waveforms surpasses the maximum digital threshold (`32767` for signed 16-bit audio). 

If clipping is left unmanaged, the values simply wrap around (`32767 + 1 = -32768`), converting what should be a loud pop into a catastrophic, ear-shattering glitch.

## Protection Strategy
The Mixer completely mitigates clipping wrap-arounds using the `audio_math_normalize_16` function. 
Every sample that exceeds the threshold is artificially clamped at the ceiling. 
While this alters the original waveform (introducing harmonic distortion known as "clipping distortion"), it prevents total math failure.

## Telemetry Visibility
Because clipping means that audio fidelity has been lost, the Mixer proactively tracks how many samples had to be clamped during the mix cycle and stores it in `g_clipped_samples`. 

During the `audio_debug_test_mixer` boot phase, 32 simultaneous Sine waves are generated and purposefully mixed to force a massive clip. The system successfully catches and clamps `1354` overflowing samples, proving the engine will remain stable under extreme duress.
