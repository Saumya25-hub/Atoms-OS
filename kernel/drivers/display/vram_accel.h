#ifndef SIGNATURES_VRAM_ACCEL_H
#define SIGNATURES_VRAM_ACCEL_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

void vram_accel_init(boot_info_t *boot_info);

#endif
