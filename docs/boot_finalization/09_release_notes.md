# Release Notes — Phase 8.4.1

## Summary

ATOMS OS has transitioned from a hardware validation environment to a production desktop operating system.

## Changes

### Removed
- `audio_test_tone.h` / `audio_test_tone.c` — Deleted permanently.
- Auto-play of `DEMO1.WAV` during `ac97_init()`.
- All verbose PCI interrupt, BAR, and codec forensic log lines from the AC97 driver.
- `audio_debug.h` include from `kernel.c`.
- All commented-out debug calls (`audio_debug_run_selftest`, `audio_debug_test_pcm_engine`, `audio_debug_test_mixer`).

### Added
- Concise production boot logs: `[AUDIO] Initializing...`, `[AUDIO] PCM Engine Ready`, `[AUDIO] Software Mixer Ready`, `[AUDIO] Ready`.
- Music Player application now invokes `audio_player_play("/DEMO1.WAV")` when launched from Start Menu or Desktop Icon.

### Preserved (Untouched)
- PCM Engine (`audio_core.c`)
- Software Mixer (`audio_mixer.c`)
- AC97 Driver (`ac97.c`, `ac97_codec.c`, `ac97_dma.c`)
- DMA Engine (`ac97_playback.c`)
- WAV Parser (`audio_player.c`)
- Audio API (`audio_api.c`)
- Ring Buffer (`audio_buffer.c`)

## Impact
- Boot time reduced (no blocking WAV playback loop).
- Desktop Shell is now the immediate boot destination.
- Audio playback is now exclusively an application-level feature.
