/*
 * BOSPECTRA V3 — Watchdog Pipeline Monitor Subsystem
 * kernel/media/bospectra/watchdog/watchdog_monitor.h
 */

#ifndef BOSPECTRA_V3_WATCHDOG_MONITOR_H
#define BOSPECTRA_V3_WATCHDOG_MONITOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STAGE_DISK = 0,
    STAGE_PACKET,
    STAGE_DECODE,
    STAGE_FRAME,
    STAGE_COLOR,
    STAGE_TEXTURE,
    STAGE_SURFACE,
    STAGE_BWE,
    STAGE_DISPLAY
} BOSPECTRA_PipelineStage;

void                    bospectra_watchdog_monitor_init(void);
void                    bospectra_watchdog_monitor_shutdown(void);

BOSPECTRA_PipelineStage bospectra_watchdog_get_current_stage(void);
const char*             bospectra_stage_to_string(BOSPECTRA_PipelineStage stage);

#endif /* BOSPECTRA_V3_WATCHDOG_MONITOR_H */
