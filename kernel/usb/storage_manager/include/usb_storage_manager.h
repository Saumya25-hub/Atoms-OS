#ifndef SIGNATURES_USB_STORAGE_MANAGER_H
#define SIGNATURES_USB_STORAGE_MANAGER_H

#include "usb_disk.h"
#include "usb_partition.h"
#include "usb_volume.h"
#include "usb_mount.h"

// API
void usb_storage_manager_init(void);
bool usb_storage_manager_register_device(usb_storage_device_t* dev);

#endif // SIGNATURES_USB_STORAGE_MANAGER_H
