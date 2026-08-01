#include "../include/usb_storage_manager.h"
#include "../include/usb_storage_debug.h"
#include "kernel/drivers/display/display.h"

static bool g_usm_initialized = false;

void usb_storage_manager_init(void) {
    if (g_usm_initialized) return;
    display_print("[USM] Initializing USB Storage Manager & VFS Integration Subsystem...\n");
    usb_disk_manager_init();
    usb_partition_manager_init();
    usb_volume_manager_init();
    usb_mount_manager_init();
    usm_telemetry_init();
    g_usm_initialized = true;
    display_print("[USM] Subsystem Initialized. Storage Manager Ready.\n");
}

bool usb_storage_manager_register_device(usb_storage_device_t* udev) {
    if (!udev) return false;
    
    usb_disk_t* disk = usb_disk_register(udev);
    if (!disk) return false;
    
    uint32_t parts = usb_partition_scan_disk(disk);
    if (parts == 0) {
        // Create default synthetic partition 1 for unpartitioned disks
        usb_partition_t synthetic_part = {
            .partition_id = 1,
            .disk = disk,
            .partition_index = 0,
            .partition_type = 0x0C, // FAT32
            .start_lba = 0,
            .sector_count = disk->total_sectors,
            .is_active = true
        };
        usb_volume_t* vol = usb_volume_create_from_partition(&synthetic_part);
        if (vol) usb_mount_volume(vol);
    } else {
        for (uint32_t i = 1; i <= parts; i++) {
            usb_partition_t* part = usb_partition_get_by_id(i);
            if (part) {
                usb_volume_t* vol = usb_volume_create_from_partition(part);
                if (vol) usb_mount_volume(vol);
            }
        }
    }
    return true;
}
