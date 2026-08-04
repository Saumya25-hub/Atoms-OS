/*
 * BOSPECTRA V3 — Watchdog History Subsystem
 * kernel/media/bospectra/watchdog/watchdog_history.h
 *
 * Ring buffer log tracking watchdog failure events and recovery resolutions.
 */

#ifndef BOSPECTRA_V3_WATCHDOG_HISTORY_H
#define BOSPECTRA_V3_WATCHDOG_HISTORY_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t timestamp_us;
    uint32_t session_id;
    char     subsystem[24];
    char     failure_reason[48];
    uint32_t recovery_level;
    bool     recovered;
} BOSPECTRA_WatchdogEvent;

void bospectra_watchdog_history_init(void);
void bospectra_watchdog_history_shutdown(void);

void bospectra_watchdog_history_record(
    uint32_t session_id,
    const char* subsystem,
    const char* failure_reason,
    uint32_t recovery_level,
    bool recovered
);

void bospectra_watchdog_history_dump(void);

#endif /* BOSPECTRA_V3_WATCHDOG_HISTORY_H */
