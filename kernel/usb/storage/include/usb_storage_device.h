#ifndef SIGNATURES_USB_STORAGE_DEVICE_H
#define SIGNATURES_USB_STORAGE_DEVICE_H

#include "../../common/usb_common.h"
#include "../../core/usb_core.h"
#include "../../pipe/usb_pipe.h"
#include "usb_scsi.h"

typedef struct {
    uint32_t            storage_id;
    uint32_t            usb_device_id;
    uint8_t             bulk_in_ep;
    uint8_t             bulk_out_ep;
    uint32_t            bulk_in_pipe;
    uint32_t            bulk_out_pipe;
    uint8_t             max_lun;
    uint32_t            total_lbas;
    uint32_t            block_size_bytes;
    bool                is_write_protected;
    bool                is_ready;
    scsi_inquiry_data_t inquiry;
    atoms_spinlock_t    lock;
} usb_storage_device_t;

// API
void usb_storage_engine_init(void);
usb_storage_device_t* usb_storage_device_create(uint32_t usb_device_id, uint8_t bulk_in_ep, uint8_t bulk_out_ep);
bool usb_storage_read_sectors(usb_storage_device_t* dev, uint32_t lba, uint16_t count, uint8_t* buffer);
bool usb_storage_write_sectors(usb_storage_device_t* dev, uint32_t lba, uint16_t count, const uint8_t* buffer);

#endif // SIGNATURES_USB_STORAGE_DEVICE_H
