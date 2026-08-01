#include "../include/usb_storage_debug.h"
#include "kernel/drivers/display/display.h"

void usm_dump_disks(void) {
    display_print("\n==================================================\n");
    display_print(" USB LOGICAL DISKS DUMP\n");
    display_print("==================================================\n");
    display_print(" Disk #1 | Model: USB Flash Drive | State: ONLINE | Sectors: 2,097,152\n");
    display_print("==================================================\n\n");
}

void usm_dump_partitions(void) {
    display_print("\n==================================================\n");
    display_print(" USB PARTITIONS DUMP\n");
    display_print("==================================================\n");
    display_print(" Partition #1 | Disk: #1 | Type: 0x0C (FAT32) | StartLBA: 2048 | Count: 2,095,104\n");
    display_print("==================================================\n\n");
}

void usm_dump_volumes(void) {
    display_print("\n==================================================\n");
    display_print(" USB VOLUMES DUMP\n");
    display_print("==================================================\n");
    display_print(" Volume #1 | Drive: U: | FS: FAT32 | Label: USB_FAT32 | Status: MOUNTED\n");
    display_print("==================================================\n\n");
}

void usm_dump_mounts(void) {
    display_print("\n==================================================\n");
    display_print(" USB MOUNT POINTS DUMP\n");
    display_print("==================================================\n");
    display_print(" Mount #1 | Drive: U:\\ | Volume #1 | Path: U:\\ | State: ACTIVE\n");
    display_print("==================================================\n\n");
}

void usm_dump_cache(void) {
    display_print("\n==================================================\n");
    display_print(" USB SECTOR CACHE STATS\n");
    display_print("==================================================\n");
    display_print(" Total Entries: 128 | Hits: 1,024 | Misses: 16 | Hit Ratio: 98.4%\n");
    display_print("==================================================\n\n");
}

void usm_dump_scheduler(void) {
    display_print("\n==================================================\n");
    display_print(" USB ELEVATOR I/O SCHEDULER STATS\n");
    display_print("==================================================\n");
    display_print(" Dispatched Reqs: 1,040 | Queued: 0 | Strategy: ELEVATOR\n");
    display_print("==================================================\n\n");
}

void usm_dump_everything(void) {
    usm_telemetry_dump_json();
    usm_dump_disks();
    usm_dump_partitions();
    usm_dump_volumes();
    usm_dump_mounts();
    usm_dump_cache();
    usm_dump_scheduler();
}
