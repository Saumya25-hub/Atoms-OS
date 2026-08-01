#include "../include/usb_disk.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_disk_t g_disk_pool[8];
static uint32_t g_disk_count = 0;
static atoms_spinlock_t g_disk_lock;

void usb_disk_manager_init(void) {
    atoms_spinlock_init(&g_disk_lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_disk_lock);
    memset(g_disk_pool, 0, sizeof(g_disk_pool));
    g_disk_count = 0;
    atoms_spin_unlock_irqrestore(&g_disk_lock, state);
    display_print("[USM DISK] Logical Disk Manager Initialized.\n");
}

usb_disk_t* usb_disk_register(usb_storage_device_t* udev) {
    if (!udev) return NULL;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_disk_lock);
    if (g_disk_count >= 8) {
        atoms_spin_unlock_irqrestore(&g_disk_lock, state);
        return NULL;
    }
    
    usb_disk_t* disk = &g_disk_pool[g_disk_count++];
    memset(disk, 0, sizeof(usb_disk_t));
    atoms_spinlock_init(&disk->lock, 0);
    disk->disk_id = g_disk_count;
    disk->udev = udev;
    disk->total_sectors = udev->total_lbas;
    disk->sector_size_bytes = udev->block_size_bytes;
    disk->state = USB_DISK_STATE_ONLINE;
    memcpy(disk->model_name, "USB Mass Storage Flash Drive", 28);
    
    display_print("[USM DISK] Registered Logical Disk #");
    display_print_dec(disk->disk_id);
    display_print(" (Sectors: ");
    display_print_dec(disk->total_sectors);
    display_print(")\n");
    
    atoms_spin_unlock_irqrestore(&g_disk_lock, state);
    return disk;
}

usb_disk_t* usb_disk_get_by_id(uint32_t disk_id) {
    if (disk_id == 0 || disk_id > g_disk_count) return NULL;
    return &g_disk_pool[disk_id - 1];
}

bool usb_disk_read_sectors(usb_disk_t* disk, uint32_t lba, uint16_t count, uint8_t* buffer) {
    if (!disk || !disk->udev || disk->state != USB_DISK_STATE_ONLINE) return false;
    return usb_storage_read_sectors(disk->udev, lba, count, buffer);
}

bool usb_disk_write_sectors(usb_disk_t* disk, uint32_t lba, uint16_t count, const uint8_t* buffer) {
    if (!disk || !disk->udev || disk->state != USB_DISK_STATE_ONLINE) return false;
    return usb_storage_write_sectors(disk->udev, lba, count, buffer);
}
