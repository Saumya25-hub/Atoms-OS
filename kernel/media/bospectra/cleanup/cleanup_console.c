/*
 * BOSPECTRA V3 — Cleanup Console Implementation
 * kernel/media/bospectra/cleanup/cleanup_console.c
 */

#include "cleanup_console.h"
#include "cleanup_metrics.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bool g_cleanup_console_initialized = false;

void bospectra_cleanup_console_init(void) {
    g_cleanup_console_initialized = true;
    bospectra_log("CLEANUP_CONSOLE", "BOSPECTRA V3 Cleanup Console Initialized.");
}

void bospectra_cleanup_console_shutdown(void) {
    g_cleanup_console_initialized = false;
}

bospectra_error_t bospectra_cleanup_console_dispatch(const char* cmd) {
    if (!cmd) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (strcmp(cmd, "cleanup") == 0 || strcmp(cmd, "cleanup metrics") == 0 || strcmp(cmd, "metrics") == 0) {
        bospectra_cleanup_metrics_dump();
        return BOSPECTRA_SUCCESS;
    }

    display_print("\nUnknown Cleanup Command: ");
    display_print(cmd);
    display_print("\nValid commands: cleanup, metrics, verify, resources, leaks\n");
    return BOSPECTRA_ERR_INVALID_ARGUMENT;
}
