# Audio Initialization Changes

The audio system was completely isolated from the automatic execution flow.

1. `kernel.c`: Removed `audio_debug_run_selftest()`, `audio_debug_test_pcm_engine()`, `audio_debug_test_mixer()`.
2. `ac97.c`: Removed `audio_player_play("/DEMO1.WAV")` from the driver init sequence.
3. `audio_core.c` / `audio_mixer.c`: Trimmed down noisy forensic logs.

The subsystem still completely boots and allocates the PCM streaming buffers, but it waits silently in `AUDIO_STATE_READY` until requested by a userland application.
