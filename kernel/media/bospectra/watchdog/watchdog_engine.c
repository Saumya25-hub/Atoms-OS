/*
 * BOSPECTRA V3 — Watchdog Engine Implementation
 * kernel/media/bospectra/watchdog/watchdog_engine.c
 */

#include "watchdog_engine.h"
#include "watchdog_rules.h"
#include "watchdog_recovery.h"
#include "watchdog_history.h"
#include "watchdog_monitor.h"
#include "../debug/bospectra_debug.h"

static bool g_watchdog_engine_initialized = false;
static PlaybackSessionCtx* g_monitored_session = NULL;

void bospectra_watchdog_engine_init(void) {
    bospectra_watchdog_history_init();
    bospectra_watchdog_recovery_init();
    bospectra_watchdog_rules_init();
    bospectra_watchdog_monitor_init();
    g_watchdog_engine_initialized = true;
    bospectra_log("WATCHDOG_ENGINE", "BOSPECTRA V3 Production Multimedia Watchdog Engine Initialized.");
}

void bospectra_watchdog_engine_shutdown(void) {
    g_watchdog_engine_initialized = false;
    g_monitored_session = NULL;
}

bospectra_error_t bospectra_watchdog_register_session(PlaybackSessionCtx* sess) {
    if (!g_watchdog_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    g_monitored_session = sess;
    return BOSPECTRA_SUCCESS;
}

void bospectra_watchdog_tick(void) {
    if (!g_watchdog_engine_initialized || !g_monitored_session) return;
    (void)bospectra_watchdog_evaluate_rules(g_monitored_session);
}
