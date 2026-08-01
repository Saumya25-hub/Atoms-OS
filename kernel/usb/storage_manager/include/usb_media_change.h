#ifndef SIGNATURES_USB_MEDIA_CHANGE_H
#define SIGNATURES_USB_MEDIA_CHANGE_H

#include "usb_disk.h"

void usb_media_change_init(void);
bool usb_media_change_poll(usb_disk_t* disk);

#endif // SIGNATURES_USB_MEDIA_CHANGE_H
