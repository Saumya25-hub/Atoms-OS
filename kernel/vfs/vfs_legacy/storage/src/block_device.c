#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include "kernel/drivers/display/display.h"
#include <stddef.h>

static BlockDevice* block_device_registry[MAX_BLOCK_DEVICES];
static int registered_device_count = 0;

void block_device_init(void) {
    for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        block_device_registry[i] = NULL;
    }
    registered_device_count = 0;
    display_print("[BLK] Block Device Layer Initialized.\n");
}

int block_device_register(BlockDevice* device) {
    if (!device) return -1;
    if (registered_device_count >= MAX_BLOCK_DEVICES) {
        display_print("[BLK] Error: Maximum block devices reached.\n");
        return -1;
    }

    int index = registered_device_count++;
    device->id = index;
    block_device_registry[index] = device;

    display_print("[BLK] Registered Device: ");
    display_print(device->name);
    display_print(" (ID: ");
    display_print_dec(index);
    display_print(")\n");

    return index;
}

BlockDevice* block_device_get(int index) {
    if (index < 0 || index >= registered_device_count) {
        return NULL;
    }
    return block_device_registry[index];
}

int block_device_count(void) {
    return registered_device_count;
}

bool block_device_read(int index, uint64_t lba, uint32_t count, void* buffer) {
    BlockDevice* dev = block_device_get(index);
    if (!dev || !dev->read || !buffer) return false;
    
    if (lba + count > dev->sector_count) {
        display_print("[BLK] Read out of bounds on device ");
        display_print_dec(index);
        display_print("\n");
        return false;
    }

    return dev->read(dev, lba, count, buffer);
}

bool block_device_write(int index, uint64_t lba, uint32_t count, void* buffer) {
    BlockDevice* dev = block_device_get(index);
    if (!dev || !dev->write || !buffer) return false;

    if (dev->read_only) {
        display_print("[BLK] Attempted to write to read-only device ");
        display_print_dec(index);
        display_print("\n");
        return false;
    }

    if (lba + count > dev->sector_count) {
        display_print("[BLK] Write out of bounds on device ");
        display_print_dec(index);
        display_print("\n");
        return false;
    }

    return dev->write(dev, lba, count, buffer);
}

bool block_device_flush(int index) {
    BlockDevice* dev = block_device_get(index);
    if (!dev) return false;

    // Flush is optional
    if (dev->flush) {
        return dev->flush(dev);
    }
    
    return true; // If no flush needed, return true
}
