#ifndef ATOMS_GPT_H
#define ATOMS_GPT_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

#define GPT_SIGNATURE   0x5452415020494645ULL /* "EFI PART" */
#define MAX_GPT_PARTS   8

/* GPT Header (92 bytes packed) */
typedef struct __attribute__((packed)) {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t size_of_partition_entry;
    uint32_t partition_entry_array_crc32;
} gpt_header_t;

/* GPT Partition Entry (128 bytes packed) */
typedef struct __attribute__((packed)) {
    uint8_t  type_guid[16];
    uint8_t  unique_guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint16_t partition_name[36];
} gpt_entry_t;

/* Discovered Partition Info */
typedef struct {
    uint32_t part_index;
    uint64_t start_lba;
    uint64_t sector_count;
    uint64_t size_mb;
    bool     is_windows_ntfs;
    bool     is_esp_fat32;
    char     name[37];
    int      bdev_id;
} GPTPartitionInfo;

/* Telemetry for Forensic Reporting */
typedef struct {
    bool             gpt_detected;
    uint64_t         disk_guid_high;
    uint64_t         disk_guid_low;
    uint32_t         partition_count;
    GPTPartitionInfo partitions[MAX_GPT_PARTS];
    int              windows_ntfs_part_idx;
    int              windows_ntfs_bdev_id;
} GPTTelemetry;

/* GPT Parser APIs */
bool gpt_scan_device(BlockDevice* parent_dev);
const GPTTelemetry* gpt_get_telemetry(void);
BlockDevice* gpt_get_windows_ntfs_bdev(void);

#endif /* ATOMS_GPT_H */
