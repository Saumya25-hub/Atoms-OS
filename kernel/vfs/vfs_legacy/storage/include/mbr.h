#ifndef MBR_H
#define MBR_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

// Standard MBR Partition Entry Structure (16 bytes)
typedef struct {
    uint8_t  status;        // 0x80 = Bootable, 0x00 = Inactive
    uint8_t  chs_first[3];  // First sector CHS address
    uint8_t  type;          // Partition type (e.g., 0x0C = FAT32 LBA)
    uint8_t  chs_last[3];   // Last sector CHS address
    uint32_t start_lba;     // LBA of first sector
    uint32_t sector_count;  // Number of sectors
} __attribute__((packed)) MBR_PartitionEntry;

// Master Boot Record Structure (512 bytes)
typedef struct {
    uint8_t  bootstrap[446];             // Bootstrap code
    MBR_PartitionEntry partitions[4];    // 4 Primary partitions
    uint16_t signature;                  // 0xAA55
} __attribute__((packed)) MBR_Sector;

void mbr_parse(int block_device_id);

#endif // MBR_H
