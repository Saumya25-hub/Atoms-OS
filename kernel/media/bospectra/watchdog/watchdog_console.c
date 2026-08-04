/*
 * BOSPECTRA V3 — Watchdog Console Implementation
 * kernel/media/bospectra/watchdog/watchdog_console.c
 */

#include "watchdog_console.h"
#include "watchdog_metrics.h"
#include "watchdog_history.h"
#include "watchdog_recovery.h"
#include "watchdog_monitor.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bool g_watchdog_console_initialized = false;

void bospectra_watchdog_console_init(void) {
    g_watchdog_console_initialized = true;
    bospectra_log("WATCHDOG_CONSOLE", "BOSPECTRA V3 Watchdog Console Initialized.");
}

void bospectra_watchdog_console_shutdown(void) {
    g_watchdog_console_initialized = false;
}

bospectra_error_t bospectra_watchdog_console_dispatch(const char* cmd) {
    if (!cmd) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (strcmp(cmd, "watchdog") == 0 || strcmp(cmd, "watchdog metrics") == 0 || strcmp(cmd, "metrics") == 0) {
        bospectra_watchdog_metrics_dump();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "watchdog history") == 0 || strcmp(cmd, "history") == 0) {
        bospectra_watchdog_history_dump();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "watchdog recover") == 0 || strcmp(cmd, "recover") == 0) {
        (void)bospectra_watchdog_execute_recovery(1, "MANUAL", "Console Command", RECOVERY_LEVEL_2_FLUSH);
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "watchdog pipeline") == 0 || strcmp(cmd, "pipeline") == 0) {
        display_print("\nWatchdog Stage Monitor: ");
        display_print(bospectra_stage_to_string(bospectra_watchdog_get_current_stage()));
        display_print("\n");
        return BOSPECTRA_SUCCESS;
    }

    display_print("\nUnknown Watchdog Command: ");
    display_print(cmd);
    display_print("\nValid commands: watchdog, metrics, history, recover, pipeline\n");
    return BOSPECTRA_ERR_INVALID_ARGUMENT;
}
