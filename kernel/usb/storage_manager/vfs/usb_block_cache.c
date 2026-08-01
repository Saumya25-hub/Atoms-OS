#include "../include/usb_disk.h"
#include "kernel/core/lib/include/string.h"

extern void usb_sector_cache_init(void);

void usb_block_cache_init(void) {
    usb_sector_cache_init();
}
