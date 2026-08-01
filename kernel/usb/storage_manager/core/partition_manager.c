#include "../include/usb_partition.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_partition_t g_partition_pool[16];
static uint32_t g_partition_count = 0;
static atoms_spinlock_t g_partition_lock;

void usb_partition_manager_init(void) {
    atoms_spinlock_init(&g_partition_lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_partition_lock);
    memset(g_partition_pool, 0, sizeof(g_partition_pool));
    g_partition_count = 0;
    atoms_spin_unlock_irqrestore(&g_partition_lock, state);
    display_print("[USM PARTITION] Partition Manager Initialized.\n");
}

uint32_t usb_partition_scan_disk(usb_disk_t* disk) {
    if (!disk) return 0;
    
    static uint8_t sector_buf[512];
    if (!usb_disk_read_sectors(disk, 0, 1, sector_buf)) return 0;
    
    mbr_header_t* mbr = (mbr_header_t*)sector_buf;
    if (mbr->signature != MBR_SIGNATURE) {
        display_print("[USM PARTITION] No valid MBR signature found on Disk #");
        display_print_dec(disk->disk_id);
        display_print("\n");
        return 0;
    }
    
    uint32_t found = 0;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_partition_lock);
    for (int i = 0; i < 4; i++) {
        mbr_partition_entry_t* entry = &mbr->entries[i];
        if (entry->partition_type != 0 && entry->sector_count > 0) {
            if (g_partition_count < 16) {
                usb_partition_t* part = &g_partition_pool[g_partition_count++];
                memset(part, 0, sizeof(usb_partition_t));
                atoms_spinlock_init(&part->lock, 0);
                part->partition_id = g_partition_count;
                part->disk = disk;
                part->partition_index = i;
                part->partition_type = entry->partition_type;
                part->start_lba = entry->start_lba;
                part->sector_count = entry->sector_count;
                part->is_active = (entry->boot_indicator == 0x80);
                found++;
                
                display_print("[USM PARTITION] Detected Partition #");
                display_print_dec(part->partition_id);
                display_print(" (Type: 0x");
                display_print_hex(part->partition_type);
                display_print(" StartLBA: ");
                display_print_dec(part->start_lba);
                display_print(")\n");
            }
        }
    }
    atoms_spin_unlock_irqrestore(&g_partition_lock, state);
    return found;
}

usb_partition_t* usb_partition_get_by_id(uint32_t partition_id) {
    if (partition_id == 0 || partition_id > g_partition_count) return NULL;
    return &g_partition_pool[partition_id - 1];
}
