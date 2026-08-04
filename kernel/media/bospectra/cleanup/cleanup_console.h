/*
 * BOSPECTRA V3 — Cleanup Console Subsystem
 * kernel/media/bospectra/cleanup/cleanup_console.h
 */

#ifndef BOSPECTRA_V3_CLEANUP_CONSOLE_H
#define BOSPECTRA_V3_CLEANUP_CONSOLE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"

void              bospectra_cleanup_console_init(void);
void              bospectra_cleanup_console_shutdown(void);

bospectra_error_t bospectra_cleanup_console_dispatch(const char* cmd);

#endif /* BOSPECTRA_V3_CLEANUP_CONSOLE_H */
