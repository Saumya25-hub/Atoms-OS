#ifndef AUDIO_TEST_MODE_H
#define AUDIO_TEST_MODE_H

// ============================================================
// AUDIO_TEST_MODE
// ============================================================
// When defined, the kernel boots ONLY the minimum subsystems
// needed for audio playback debugging:
//   PMM → VMM → Heap → Interrupts → Timer → Scheduler →
//   PCI → VFS → FAT32 → AC97 → Audio Service
//
// All GUI, Desktop, Shell, BWE, AGDTE, Horse Engine, ROOK,
// compositor, and window manager services are DISABLED.
//
// To enable:  #define AUDIO_TEST_MODE_ENABLED 1
// To disable: Comment out or set to 0
// ============================================================

#define AUDIO_TEST_MODE_ENABLED 1

// Called from kernel_main when AUDIO_TEST_MODE_ENABLED is 1.
// Sets up audio subsystem and starts DEMO1.WAV with full telemetry.
void audio_test_mode_entry(void);

// Periodic telemetry dump — called from the audio service loop
// in test mode to print pipeline state every N iterations.
void audio_test_mode_telemetry_tick(void);

#endif // AUDIO_TEST_MODE_H
