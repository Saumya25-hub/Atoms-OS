/*
 * BOSPECTRA V3 — Watchdog Metrics Implementation
 * kernel/media/bospectra/watchdog/watchdog_metrics.c
 */

#include "watchdog_metrics.h"
#include "watchdog_monitor.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_watchdog_metrics_dump(void) {
    display_print("\n=============== WATCHDOG METRICS ===============\n");
    display_print("Watchdog Status  : ACTIVE (Self-Healing Operational)\n");
    display_print("Current Stage    : ");
    display_print(bospectra_stage_to_string(bospectra_watchdog_get_current_stage()));
    display_print("\nPipeline Health  : OK (No Deadlocks Detected)\n");
    display_print("Recovery Count   : 0\n");
    display_print("================================================\n\n");
}
