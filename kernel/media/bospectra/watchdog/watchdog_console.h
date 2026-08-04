/*
 * BOSPECTRA V3 — Watchdog Console Subsystem
 * kernel/media/bospectra/watchdog/watchdog_console.h
 */

#ifndef BOSPECTRA_V3_WATCHDOG_CONSOLE_H
#define BOSPECTRA_V3_WATCHDOG_CONSOLE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"

void              bospectra_watchdog_console_init(void);
void              bospectra_watchdog_console_shutdown(void);

bospectra_error_t bospectra_watchdog_console_dispatch(const char* cmd);

#endif /* BOSPECTRA_V3_WATCHDOG_CONSOLE_H */
