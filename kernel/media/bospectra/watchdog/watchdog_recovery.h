/*
 * BOSPECTRA V3 — Watchdog Recovery Subsystem
 * kernel/media/bospectra/watchdog/watchdog_recovery.h
 */

#ifndef BOSPECTRA_V3_WATCHDOG_RECOVERY_H
#define BOSPECTRA_V3_WATCHDOG_RECOVERY_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RECOVERY_LEVEL_1_RETRY = 1,
    RECOVERY_LEVEL_2_FLUSH,
    RECOVERY_LEVEL_3_RESET_SUBSYSTEM,
    RECOVERY_LEVEL_4_RESTART_SESSION,
    RECOVERY_LEVEL_5_NOTIFY_APP,
    RECOVERY_LEVEL_6_KERNEL_PANIC
} BOSPECTRA_RecoveryLevel;

void              bospectra_watchdog_recovery_init(void);
void              bospectra_watchdog_recovery_shutdown(void);

bospectra_error_t bospectra_watchdog_execute_recovery(
    uint32_t session_id,
    const char* subsystem,
    const char* failure_reason,
    BOSPECTRA_RecoveryLevel level
);

#endif /* BOSPECTRA_V3_WATCHDOG_RECOVERY_H */
