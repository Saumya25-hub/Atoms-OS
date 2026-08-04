/*
 * BOSPECTRA V3 — Diagnostic Console Subsystem
 * kernel/media/bospectra/diagnostics/diagnostic_console.h
 *
 * Dedicated CLI diagnostic dispatcher for BOSPECTRA commands.
 */

#ifndef BOSPECTRA_V3_DIAGNOSTIC_CONSOLE_H
#define BOSPECTRA_V3_DIAGNOSTIC_CONSOLE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"

void              bospectra_diagnostic_console_init(void);
void              bospectra_diagnostic_console_shutdown(void);

bospectra_error_t bospectra_diag_dispatch_command(const char* cmd);

#endif /* BOSPECTRA_V3_DIAGNOSTIC_CONSOLE_H */
