#ifndef SIGNATURES_SYSTEM_POWER_H
#define SIGNATURES_SYSTEM_POWER_H

#include <stdint.h>
#include <stdbool.h>

extern volatile bool g_system_power_transitioning;

/* High-Level Graceful Power Entrypoints (UI / Start Menu / Shell) */
void atoms_power_shutdown(void);
void atoms_power_reboot(void);

/* Low-Level Direct Bare-Metal Hardware Power Routines */
void system_shutdown(void);
void system_reboot(void);

void remote_power_init(void);

#endif // SIGNATURES_SYSTEM_POWER_H
