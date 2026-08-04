/*
 * BOSPECTRA V3 — Watchdog Recovery Implementation
 * kernel/media/bospectra/watchdog/watchdog_recovery.c
 */

#include "watchdog_recovery.h"
#include "watchdog_history.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

static bool g_watchdog_rec_initialized = false;

void bospectra_watchdog_recovery_init(void) {
    g_watchdog_rec_initialized = true;
    bospectra_log("WATCHDOG_RECOVERY", "BOSPECTRA V3 Watchdog Recovery Engine Initialized.");
}

void bospectra_watchdog_recovery_shutdown(void) {
    g_watchdog_rec_initialized = false;
}

bospectra_error_t bospectra_watchdog_execute_recovery(
    uint32_t session_id,
    const char* subsystem,
    const char* failure_reason,
    BOSPECTRA_RecoveryLevel level
) {
    if (!g_watchdog_rec_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    display_print("\n====================================\n");
    display_print("WATCHDOG RECOVERY EXECUTED\n");
    display_print("Subsystem        : ");
    display_print(subsystem ? subsystem : "UNKNOWN");
    display_print("\nFailure          : ");
    display_print(failure_reason ? failure_reason : "DEADLOCK");
    display_print("\nRecovery Level   : Level ");
    bospectra_trace_u32("Lvl", (uint32_t)level);
    display_print("====================================\n\n");

    bospectra_watchdog_history_record(session_id, subsystem, failure_reason, (uint32_t)level, true);
    return BOSPECTRA_SUCCESS;
}
