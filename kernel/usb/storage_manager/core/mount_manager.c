#include "../include/usb_mount.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_mount_point_t g_mount_pool[16];
static uint32_t g_mount_count = 0;
static atoms_spinlock_t g_mount_lock;

void usb_mount_manager_init(void) {
    atoms_spinlock_init(&g_mount_lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_mount_lock);
    memset(g_mount_pool, 0, sizeof(g_mount_pool));
    g_mount_count = 0;
    atoms_spin_unlock_irqrestore(&g_mount_lock, state);
    display_print("[USM MOUNT] Mount Manager Initialized.\n");
}

bool usb_mount_volume(usb_volume_t* vol) {
    if (!vol) return false;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_mount_lock);
    if (g_mount_count >= 16) {
        atoms_spin_unlock_irqrestore(&g_mount_lock, state);
        return false;
    }
    
    usb_mount_point_t* mnt = &g_mount_pool[g_mount_count++];
    memset(mnt, 0, sizeof(usb_mount_point_t));
    atoms_spinlock_init(&mnt->lock, 0);
    mnt->mount_id = g_mount_count;
    mnt->volume = vol;
    mnt->is_mounted = true;
    vol->is_mounted = true;
    
    mnt->mount_path[0] = vol->drive_letter;
    mnt->mount_path[1] = ':';
    mnt->mount_path[2] = '\\';
    mnt->mount_path[3] = '\0';
    
    display_print("[USM MOUNT] Mounted Volume Drive ");
    display_print(mnt->mount_path);
    display_print(" Successfully.\n");
    
    atoms_spin_unlock_irqrestore(&g_mount_lock, state);
    return true;
}

bool usb_unmount_volume(usb_volume_t* vol) {
    if (!vol || !vol->is_mounted) return false;
    vol->is_mounted = false;
    char letter_buf[2] = { vol->drive_letter, '\0' };
    display_print("[USM MOUNT] Unmounted Volume Drive ");
    display_print(letter_buf);
    display_print(":\n");
    return true;
}

bool usb_safe_remove_volume(usb_volume_t* vol) {
    if (!vol) return false;
    char letter_buf[2] = { vol->drive_letter, '\0' };
    display_print("[USM SAFE REMOVE] Safely Ejecting Volume Drive ");
    display_print(letter_buf);
    display_print(": ...\n");
    usb_unmount_volume(vol);
    display_print("[USM SAFE REMOVE] Drive ");
    display_print(letter_buf);
    display_print(": is Safe to Disconnect.\n");
    return true;
}
