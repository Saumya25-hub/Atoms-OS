#ifndef SIGNATURES_USB_MOUNT_H
#define SIGNATURES_USB_MOUNT_H

#include "usb_volume.h"

typedef struct {
    uint32_t         mount_id;
    usb_volume_t*    volume;
    char             mount_path[32]; // e.g. "/mnt/usb_u" or "U:\"
    bool             is_mounted;
    atoms_spinlock_t lock;
} usb_mount_point_t;

// API
void usb_mount_manager_init(void);
bool usb_mount_volume(usb_volume_t* vol);
bool usb_unmount_volume(usb_volume_t* vol);
bool usb_safe_remove_volume(usb_volume_t* vol);

#endif // SIGNATURES_USB_MOUNT_H
