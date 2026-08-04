/*
 * BOSPECTRA V3 — Runtime Health Subsystem
 * kernel/media/bospectra/runtime/runtime_health.h
 */

#ifndef BOSPECTRA_V3_RUNTIME_HEALTH_H
#define BOSPECTRA_V3_RUNTIME_HEALTH_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    HEALTH_STATE_OK = 0,
    HEALTH_STATE_WARNING,
    HEALTH_STATE_DEGRADED,
    HEALTH_STATE_FAILED
} BOSPECTRA_HealthState;

void                  bospectra_runtime_health_init(void);
void                  bospectra_runtime_health_shutdown(void);

BOSPECTRA_HealthState bospectra_runtime_health_get_state(void);
const char*           bospectra_health_to_string(BOSPECTRA_HealthState state);

#endif /* BOSPECTRA_V3_RUNTIME_HEALTH_H */
