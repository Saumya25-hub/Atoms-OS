#include "kernel/vfs/vfs_legacy/storage/include/mbr.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/vfs/vfs_legacy/storage/include/disk_manager.h"

// Note: In Sprint 4 (Disk Manager) we will create a global partition registry.
// For Sprint 3, we just parse and print/validate.

void mbr_parse(int block_device_id) {
    display_print("\n[MBR] Reading Sector 0...\n");

    MBR_Sector* sector = (MBR_Sector*)kmalloc(512);
    if (!sector) {
        display_print("[MBR] Error: kmalloc failed\n");
        return;
    }

    if (!block_device_read(block_device_id, 0, 1, sector)) {
        display_print("[MBR] Error: Failed to read from Block Device Layer\n");
        // We cannot free yet because kfree isn't implemented, but that's fine for now
        return;
    }

    if (sector->signature == 0xAA55) {
        display_print("[MBR] Signature 55AA PASS\n");
    } else {
        display_print("[MBR] Signature 55AA FAILED (Found 0x");
        display_print_hex(sector->signature);
        display_print(")\n");
        return;
    }

    int active_partitions = 0;
    for (int i = 0; i < 4; i++) {
        if (sector->partitions[i].type != 0x00) { // Type 0x00 is empty
            active_partitions++;
        }
    }

    display_print("[MBR] Partition Count : ");
    display_print_dec(active_partitions);
    display_print("\n");

    for (int i = 0; i < 4; i++) {
        if (sector->partitions[i].type != 0x00) {
            display_print("\n[MBR] Partition ");
            display_print_dec(i + 1);
            display_print("\n");
            
            display_print("Bootable : ");
            if (sector->partitions[i].status == 0x80) {
                display_print("Yes\n");
            } else {
                display_print("No\n");
            }
            
            display_print("Type : 0x");
            display_print_hex(sector->partitions[i].type);
            display_print("\n");
            
            display_print("Start LBA : ");
            display_print_dec(sector->partitions[i].start_lba);
            display_print("\n");
            
            display_print("Sector Count : ");
            display_print_dec(sector->partitions[i].sector_count);
            display_print("\n");
            
            // Register with Disk Manager
            disk_manager_register_partition(
                block_device_id,
                sector->partitions[i].start_lba,
                sector->partitions[i].sector_count,
                sector->partitions[i].type
            );
        }
    }
    
    display_print("\n[MBR] PASS\n");
    
    // Memory leak expected until kfree is built, completely fine for validation.
}
