/*
 * BOSPECTRA V3 — Media Debugger Subsystem
 * kernel/media/bospectra/diagnostics/media_debugger.h
 *
 * Core memory and object inspector.
 */

#ifndef BOSPECTRA_V3_MEDIA_DEBUGGER_H
#define BOSPECTRA_V3_MEDIA_DEBUGGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void bospectra_media_debugger_init(void);
void bospectra_media_debugger_shutdown(void);

void bospectra_media_debugger_inspect_all(void);

#endif /* BOSPECTRA_V3_MEDIA_DEBUGGER_H */
