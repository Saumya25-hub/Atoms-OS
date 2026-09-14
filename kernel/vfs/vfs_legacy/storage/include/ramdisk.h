#ifndef RAMDISK_H
#define RAMDISK_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

int ramdisk_init(uint64_t base_phys, uint64_t size_bytes);
BlockDevice* ramdisk_get_device(void);

#endif // RAMDISK_H
