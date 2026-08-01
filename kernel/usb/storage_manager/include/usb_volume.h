#ifndef SIGNATURES_USB_VOLUME_H
#define SIGNATURES_USB_VOLUME_H

#include "usb_partition.h"

typedef enum {
    FS_TYPE_UNKNOWN = 0,
    FS_TYPE_FAT32,
    FS_TYPE_NTFS,
    FS_TYPE_EXFAT,
    FS_TYPE_EXT4
} usb_fs_type_t;

typedef struct {
    uint32_t         volume_id;
    usb_partition_t* partition;
    char             drive_letter; // e.g. 'U', 'V', 'W', 'X'
    usb_fs_type_t    fs_type;
    char             label[16];
    bool             is_mounted;
    atoms_spinlock_t lock;
} usb_volume_t;

// API
void usb_volume_manager_init(void);
usb_volume_t* usb_volume_create_from_partition(usb_partition_t* part);
usb_volume_t* usb_volume_get_by_letter(char drive_letter);
const char* usb_fs_type_to_string(usb_fs_type_t fs);

#endif // SIGNATURES_USB_VOLUME_H
