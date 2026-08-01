#include "../include/usb_volume.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_volume_t g_volume_pool[16];
static uint32_t g_volume_count = 0;
static atoms_spinlock_t g_volume_lock;
static char g_next_letter = 'U';

void usb_volume_manager_init(void) {
    atoms_spinlock_init(&g_volume_lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_volume_lock);
    memset(g_volume_pool, 0, sizeof(g_volume_pool));
    g_volume_count = 0;
    g_next_letter = 'U';
    atoms_spin_unlock_irqrestore(&g_volume_lock, state);
    display_print("[USM VOLUME] Volume Manager Initialized.\n");
}

usb_volume_t* usb_volume_create_from_partition(usb_partition_t* part) {
    if (!part) return NULL;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_volume_lock);
    if (g_volume_count >= 16) {
        atoms_spin_unlock_irqrestore(&g_volume_lock, state);
        return NULL;
    }
    
    usb_volume_t* vol = &g_volume_pool[g_volume_count++];
    memset(vol, 0, sizeof(usb_volume_t));
    atoms_spinlock_init(&vol->lock, 0);
    vol->volume_id = g_volume_count;
    vol->partition = part;
    vol->drive_letter = g_next_letter++;
    if (g_next_letter > 'Z') g_next_letter = 'U';
    
    if (part->partition_type == 0x0B || part->partition_type == 0x0C) {
        vol->fs_type = FS_TYPE_FAT32;
        memcpy(vol->label, "USB_FAT32", 9);
    } else if (part->partition_type == 0x07) {
        vol->fs_type = FS_TYPE_NTFS;
        memcpy(vol->label, "USB_NTFS", 8);
    } else {
        vol->fs_type = FS_TYPE_UNKNOWN;
        memcpy(vol->label, "USB_DATA", 8);
    }
    
    vol->is_mounted = false;
    
    char letter_buf[2] = { vol->drive_letter, '\0' };
    display_print("[USM VOLUME] Created Volume #");
    display_print_dec(vol->volume_id);
    display_print(" Drive ");
    display_print(letter_buf);
    display_print(": [");
    display_print(usb_fs_type_to_string(vol->fs_type));
    display_print("]\n");
    
    atoms_spin_unlock_irqrestore(&g_volume_lock, state);
    return vol;
}

usb_volume_t* usb_volume_get_by_letter(char drive_letter) {
    for (uint32_t i = 0; i < g_volume_count; i++) {
        if (g_volume_pool[i].drive_letter == drive_letter) {
            return &g_volume_pool[i];
        }
    }
    return NULL;
}

const char* usb_fs_type_to_string(usb_fs_type_t fs) {
    switch (fs) {
        case FS_TYPE_FAT32: return "FAT32";
        case FS_TYPE_NTFS:  return "NTFS";
        case FS_TYPE_EXFAT: return "exFAT";
        case FS_TYPE_EXT4:  return "EXT4";
        default: return "UNKNOWN";
    }
}
