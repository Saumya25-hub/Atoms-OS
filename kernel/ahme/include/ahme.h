#ifndef ATOMS_AHME_H
#define ATOMS_AHME_H

#include <stdint.h>
#include <stdbool.h>

#include "ahme_profile.h"
#include "ahme_policy.h"
#include "ahme_quirks.h"
#include "ahme_health.h"
#include "ahme_input.h"

typedef struct {
    bool is_initialized;
    AHMEHardwareProfile profile;
    uint32_t active_quirks_mask;
    uint64_t uptime_ticks;
} AHMEState;

/* Master AHME API */
void ahme_init(void);
AHMEState* ahme_get_state(void);
void ahme_log_telemetry(void);

#endif // ATOMS_AHME_H
