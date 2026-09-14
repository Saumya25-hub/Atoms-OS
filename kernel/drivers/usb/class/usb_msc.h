#ifndef ATOMS_USB_MSC_H
#define ATOMS_USB_MSC_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/drivers/usb/core/usb_core.h"
#include "third_party/usb/include/usb_msc_defs.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

#define USB_MSC_MAX_DEVICES 8

typedef struct {
    bool is_active;
    USBDevice* usb_dev;
    uint8_t slot_id;
    uint8_t interface_number;
    uint8_t bulk_in_ep;
    uint16_t bulk_in_max_packet;
    uint8_t bulk_out_ep;
    uint16_t bulk_out_max_packet;
    uint8_t max_lun;
    uint32_t block_size;
    uint64_t total_blocks;
    char vendor_id[9];
    char product_id[17];
    char product_rev[5];
    int block_device_id;
    BlockDevice bdev;
} usb_msc_device_t;

void usb_msc_init(void);
void usb_msc_register_block_devices(void);
usb_msc_device_t* usb_msc_get_device(uint32_t index);
uint32_t usb_msc_get_device_count(void);

// SCSI command execution over Bulk-Only Transport
bool usb_msc_scsi_test_unit_ready(usb_msc_device_t* msc);
bool usb_msc_scsi_request_sense(usb_msc_device_t* msc);
bool usb_msc_scsi_inquiry(usb_msc_device_t* msc);
bool usb_msc_scsi_read_capacity(usb_msc_device_t* msc);
bool usb_msc_scsi_read10(usb_msc_device_t* msc, uint64_t lba, uint32_t count, void* buffer);
bool usb_msc_scsi_write10(usb_msc_device_t* msc, uint64_t lba, uint32_t count, const void* buffer);

#endif // ATOMS_USB_MSC_H
