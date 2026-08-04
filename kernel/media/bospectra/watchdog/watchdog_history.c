/*
 * BOSPECTRA V3 — Watchdog History Implementation
 * kernel/media/bospectra/watchdog/watchdog_history.c
 */

#include "watchdog_history.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_WATCHDOG_RING_CAPACITY 32

extern void display_print(const char* str);

static BOSPECTRA_WatchdogEvent g_watchdog_ring[BOSPECTRA_WATCHDOG_RING_CAPACITY];
static uint32_t                g_watchdog_head = 0;
static bool                    g_watchdog_history_initialized = false;

void bospectra_watchdog_history_init(void) {
    memset(g_watchdog_ring, 0, sizeof(g_watchdog_ring));
    g_watchdog_head = 0;
    g_watchdog_history_initialized = true;
    bospectra_log("WATCHDOG_HISTORY", "BOSPECTRA V3 Watchdog History Log Initialized.");
}

void bospectra_watchdog_history_shutdown(void) {
    g_watchdog_history_initialized = false;
}

void bospectra_watchdog_history_record(
    uint32_t session_id,
    const char* subsystem,
    const char* failure_reason,
    uint32_t recovery_level,
    bool recovered
) {
    if (!g_watchdog_history_initialized) return;

    BOSPECTRA_WatchdogEvent* slot = &g_watchdog_ring[g_watchdog_head];
    slot->timestamp_us = 1000U;
    slot->session_id = session_id;
    if (subsystem) strncpy(slot->subsystem, subsystem, sizeof(slot->subsystem) - 1);
    if (failure_reason) strncpy(slot->failure_reason, failure_reason, sizeof(slot->failure_reason) - 1);
    slot->recovery_level = recovery_level;
    slot->recovered = recovered;

    g_watchdog_head = (g_watchdog_head + 1) % BOSPECTRA_WATCHDOG_RING_CAPACITY;
}

void bospectra_watchdog_history_dump(void) {
    if (!g_watchdog_history_initialized) return;

    display_print("\n============= WATCHDOG FAILURE & RECOVERY HISTORY =============\n");
    for (size_t i = 0; i < BOSPECTRA_WATCHDOG_RING_CAPACITY; i++) {
        if (g_watchdog_ring[i].session_id > 0 || g_watchdog_ring[i].subsystem[0] != '\0') {
            display_print("Sess:");
            bospectra_trace_u32("ID", g_watchdog_ring[i].session_id);
            display_print(" [");
            display_print(g_watchdog_ring[i].subsystem);
            display_print("] Failure: ");
            display_print(g_watchdog_ring[i].failure_reason);
            display_print(" | Level: ");
            bospectra_trace_u32("Lvl", g_watchdog_ring[i].recovery_level);
            display_print(" | Recovered: ");
            display_print(g_watchdog_ring[i].recovered ? "YES\n" : "NO\n");
        }
    }
    display_print("===============================================================\n");
}
