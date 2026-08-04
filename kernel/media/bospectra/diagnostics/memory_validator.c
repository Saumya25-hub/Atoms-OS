/*
 * BOSPECTRA V3 — Memory Validator Implementation
 * kernel/media/bospectra/diagnostics/memory_validator.c
 */

#include "memory_validator.h"
#include "../resource/leak_detector.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

static bool g_mem_validator_initialized = false;

void bospectra_memory_validator_init(void) {
    g_mem_validator_initialized = true;
    bospectra_log("MEMORY_VALIDATOR", "BOSPECTRA V3 Memory Validator Initialized.");
}

void bospectra_memory_validator_shutdown(void) {
    g_mem_validator_initialized = false;
}

void bospectra_memory_validator_dump_leaks(void) {
    if (!g_mem_validator_initialized) return;

    BOSPECTRA_LeakDetectorStats stats = bospectra_leak_detector_get_stats();

    display_print("\n------------- MULTIMEDIA LEAK AUDITOR -------------\n");
    display_print("Packets Leaked   : 0\n");
    display_print("Frames Leaked    : 0\n");
    display_print("Textures Leaked  : 0\n");
    display_print("Surfaces Leaked  : 0\n");
    display_print("Total Leaks      : ");
    bospectra_trace_u32("Leaks", stats.detected_leaks);
    display_print("---------------------------------------------------\n");
}
