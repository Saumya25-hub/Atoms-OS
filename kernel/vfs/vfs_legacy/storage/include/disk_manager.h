#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

#define MAX_LOGICAL_DRIVES 32

// Represents a logical partition exposed as a standalone Block Device
typedef struct {
    int parent_device_id;
    uint64_t start_lba;
    uint64_t sector_count;
    uint8_t partition_type;
} LogicalDriveData;

void disk_manager_init(void);

// Called by MBR/GPT parsers to register a discovered partition
int disk_manager_register_partition(int parent_device_id, uint64_t start_lba, uint64_t sector_count, uint8_t type);

#endif // DISK_MANAGER_H
