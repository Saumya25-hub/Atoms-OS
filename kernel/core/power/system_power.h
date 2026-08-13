#ifndef SIGNATURES_SYSTEM_POWER_H
#define SIGNATURES_SYSTEM_POWER_H

#include <stdint.h>
#include <stdbool.h>

void system_shutdown(void);
void system_reboot(void);
void remote_power_init(void);

#endif
