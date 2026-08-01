#ifndef SIGNATURES_USB_PARTITION_H
#define SIGNATURES_USB_PARTITION_H

#include "usb_disk.h"

#define MBR_SIGNATURE 0xAA55

// MBR Partition Entry (16 bytes packed)
typedef struct __attribute__((packed)) {
    uint8_t  boot_indicator; // 0x80 = Active
    uint8_t  start_head;
    uint8_t  start_sector_cylinder[2];
    uint8_t  partition_type; // 0x0B/0x0C = FAT32, 0x07 = NTFS, 0x83 = Linux
    uint8_t  end_head;
    uint8_t  end_sector_cylinder[2];
    uint32_t start_lba;
    uint32_t sector_count;
} mbr_partition_entry_t;

// Master Boot Record Structure (512 bytes packed)
typedef struct __attribute__((packed)) {
    uint8_t               boot_code[446];
    mbr_partition_entry_t entries[4];
    uint16_t              signature; // 0xAA55
} mbr_header_t;

typedef struct {
    uint32_t         partition_id;
    usb_disk_t*      disk;
    uint32_t         partition_index;
    uint8_t          partition_type;
    uint32_t         start_lba;
    uint32_t         sector_count;
    bool             is_active;
    atoms_spinlock_t lock;
} usb_partition_t;

// API
void usb_partition_manager_init(void);
uint32_t usb_partition_scan_disk(usb_disk_t* disk);
usb_partition_t* usb_partition_get_by_id(uint32_t partition_id);

#endif // SIGNATURES_USB_PARTITION_H
