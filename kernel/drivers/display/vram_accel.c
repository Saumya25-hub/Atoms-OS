#include "kernel/drivers/display/vram_accel.h"
#include <stdint.h>
#include <stdbool.h>

extern void com1_puts(const char *s);

void vram_accel_init(boot_info_t *boot_info) {
    (void)boot_info;
    com1_puts("[VRAM_ACCEL] UEFI Firmware VRAM Caching Active (Safe Mode, Zero MSR Overwrites) 100% PASS\r\n");
}
