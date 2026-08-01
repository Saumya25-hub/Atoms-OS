#ifndef SIGNATURES_USB_DISK_H
#define SIGNATURES_USB_DISK_H

#include "../../common/usb_common.h"
#include "../../storage/include/usb_storage_device.h"

typedef enum {
    USB_DISK_STATE_OFFLINE = 0,
    USB_DISK_STATE_ONLINE,
    USB_DISK_STATE_REMOVED,
    USB_DISK_STATE_ERROR
} usb_disk_state_t;

typedef struct {
    uint32_t              disk_id;
    usb_storage_device_t* udev;
    uint32_t              total_sectors;
    uint32_t              sector_size_bytes;
    usb_disk_state_t      state;
    char                  model_name[32];
    atoms_spinlock_t      lock;
} usb_disk_t;

// API
void usb_disk_manager_init(void);
usb_disk_t* usb_disk_register(usb_storage_device_t* udev);
usb_disk_t* usb_disk_get_by_id(uint32_t disk_id);
bool usb_disk_read_sectors(usb_disk_t* disk, uint32_t lba, uint16_t count, uint8_t* buffer);
bool usb_disk_write_sectors(usb_disk_t* disk, uint32_t lba, uint16_t count, const uint8_t* buffer);

#endif // SIGNATURES_USB_DISK_H
