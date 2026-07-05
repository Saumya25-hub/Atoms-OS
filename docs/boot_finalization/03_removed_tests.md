# Removed Tests

The following temporary validations have been fully deleted from the OS tree:

- `kernel/audio/audio_test_tone.c`
- `kernel/audio/audio_test_tone.h`

These files previously bypassed the VFS and Mixer to push synthetic triangle/sine waves directly into the AC97 driver during early hardware bring-up. They are no longer needed.
