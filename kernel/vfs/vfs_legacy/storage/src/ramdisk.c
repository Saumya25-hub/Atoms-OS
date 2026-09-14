#include "kernel/vfs/vfs_legacy/storage/include/ramdisk.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include "kernel/drivers/display/display.h"
#include <stddef.h>

static BlockDevice s_ramdisk_device;
static uint64_t s_ramdisk_base = 0;
static uint64_t s_ramdisk_size = 0;
static bool s_ramdisk_initialized = false;

static bool ramdisk_read(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    (void)dev;
    if (!s_ramdisk_initialized || !buffer) return false;
    uint64_t offset = lba * 512ULL;
    uint64_t bytes = (uint64_t)count * 512ULL;
    if (offset + bytes > s_ramdisk_size) {
        display_print("[RAMDISK] Read out of bounds\n");
        return false;
    }
    const uint8_t* src = (const uint8_t*)(uintptr_t)(s_ramdisk_base + offset);
    uint8_t* dst = (uint8_t*)buffer;
    for (uint64_t i = 0; i < bytes; i++) {
        dst[i] = src[i];
    }
    return true;
}

static bool ramdisk_write(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    (void)dev;
    if (!s_ramdisk_initialized || !buffer) return false;
    uint64_t offset = lba * 512ULL;
    uint64_t bytes = (uint64_t)count * 512ULL;
    if (offset + bytes > s_ramdisk_size) {
        display_print("[RAMDISK] Write out of bounds\n");
        return false;
    }
    const uint8_t* src = (const uint8_t*)buffer;
    uint8_t* dst = (uint8_t*)(uintptr_t)(s_ramdisk_base + offset);
    for (uint64_t i = 0; i < bytes; i++) {
        dst[i] = src[i];
    }
    return true;
}

static bool ramdisk_flush(BlockDevice* dev) {
    (void)dev;
    return true;
}

int ramdisk_init(uint64_t base_phys, uint64_t size_bytes) {
    if (base_phys == 0 || size_bytes < 512) {
        return -1;
    }
    s_ramdisk_base = base_phys;
    s_ramdisk_size = size_bytes;

    s_ramdisk_device.id = 0;
    s_ramdisk_device.name = "ramdisk";
    s_ramdisk_device.sector_size = 512;
    s_ramdisk_device.sector_count = size_bytes / 512ULL;
    s_ramdisk_device.read_only = false;
    s_ramdisk_device.driver_data = (void*)(uintptr_t)base_phys;
    s_ramdisk_device.read = ramdisk_read;
    s_ramdisk_device.write = ramdisk_write;
    s_ramdisk_device.flush = ramdisk_flush;

    int bd_id = block_device_register(&s_ramdisk_device);
    if (bd_id >= 0) {
        s_ramdisk_initialized = true;
        display_print("[RAMDISK] Online: base=0x");
        display_print_hex(base_phys);
        display_print(" size=");
        display_print_dec((uint32_t)(size_bytes / (1024 * 1024)));
        display_print("MB sectors=");
        display_print_dec((uint32_t)s_ramdisk_device.sector_count);
        display_print(" (Block ID: ");
        display_print_dec(bd_id);
        display_print(")\n");
    }
    return bd_id;
}

BlockDevice* ramdisk_get_device(void) {
    return s_ramdisk_initialized ? &s_ramdisk_device : NULL;
}
