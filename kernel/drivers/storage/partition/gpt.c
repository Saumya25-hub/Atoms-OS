#include "gpt.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void com1_puts(const char* s);

static const uint8_t GUID_BASIC_DATA[16] = {
    0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
    0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7
};

static const uint8_t GUID_EFI_SYSTEM[16] = {
    0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
    0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B
};

typedef struct {
    BlockDevice* parent;
    uint64_t     start_lba;
    uint64_t     sector_count;
} GPTPartContext;

static GPTPartContext s_part_contexts[MAX_GPT_PARTS];
static BlockDevice    s_part_bdevs[MAX_GPT_PARTS];
static char           s_part_names[MAX_GPT_PARTS][16];
static GPTTelemetry   s_gpt_telemetry;
static int            s_win_bdev_id = -1;

static void gpt_dbg_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    com1_puts(&buf[pos + 1]);
}

static void gpt_dbg_hex(uint64_t val) {
    com1_puts("0x");
    if (val == 0) { com1_puts("0"); return; }
    char buf[20]; int pos = 18; buf[19] = '\0';
    const char hex[] = "0123456789ABCDEF";
    while (val > 0) { buf[pos--] = hex[val & 0xF]; val >>= 4; }
    com1_puts(&buf[pos + 1]);
}

/* Partition Sub-BlockDevice Read (with strict boundary clamping) */
static bool gpt_part_read(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    if (!dev || !dev->driver_data || !buffer || count == 0) return false;
    GPTPartContext* ctx = (GPTPartContext*)dev->driver_data;

    /* Boundary clamping */
    if (lba + count > ctx->sector_count) {
        com1_puts("[GPT] Error: Read out of partition bounds!\r\n");
        return false;
    }

    return ctx->parent->read(ctx->parent, ctx->start_lba + lba, count, buffer);
}

/* Partition Sub-BlockDevice Write (with strict boundary clamping) */
static bool gpt_part_write(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    if (!dev || !dev->driver_data || !buffer || count == 0) return false;
    GPTPartContext* ctx = (GPTPartContext*)dev->driver_data;

    /* Strict boundary clamping: physical protection of other partitions and disk headers */
    if (lba + count > ctx->sector_count) {
        com1_puts("[GPT] Error: Write out of partition bounds! Operation blocked.\r\n");
        return false;
    }

    return ctx->parent->write(ctx->parent, ctx->start_lba + lba, count, buffer);
}

/* Partition Sub-BlockDevice Flush */
static bool gpt_part_flush(struct BlockDevice* dev) {
    if (!dev || !dev->driver_data) return false;
    GPTPartContext* ctx = (GPTPartContext*)dev->driver_data;
    if (ctx->parent->flush) {
        return ctx->parent->flush(ctx->parent);
    }
    return true;
}

bool gpt_scan_device(BlockDevice* parent_dev) {
    if (!parent_dev || !parent_dev->read) return false;

    com1_puts("[GPT] Scanning Partition Table on device ");
    com1_puts(parent_dev->name ? parent_dev->name : "unnamed");
    com1_puts("...\r\n");

    memset(&s_gpt_telemetry, 0, sizeof(s_gpt_telemetry));
    s_gpt_telemetry.windows_ntfs_part_idx = -1;
    s_gpt_telemetry.windows_ntfs_bdev_id = -1;
    s_win_bdev_id = -1;

    uint32_t sec_size = (uint32_t)parent_dev->sector_size;
    if (sec_size < 512) sec_size = 512;

    uint8_t* sec_buf = (uint8_t*)kmalloc(sec_size);
    if (!sec_buf) {
        com1_puts("[GPT] Error: Memory allocation failed for sector buffer!\r\n");
        return false;
    }

    /* STEP 1: Read LBA 1 (Primary GPT Header) */
    if (!parent_dev->read(parent_dev, 1, 1, sec_buf)) {
        com1_puts("[GPT] Error: Failed to read LBA 1 (GPT Header)!\r\n");
        kfree(sec_buf);
        return false;
    }

    gpt_header_t* gpt_hdr = (gpt_header_t*)sec_buf;
    if (gpt_hdr->signature != GPT_SIGNATURE) {
        com1_puts("[GPT] No GPT signature at LBA 1 (Got ");
        gpt_dbg_hex(gpt_hdr->signature); com1_puts(")\r\n");

        /* Check MBR Fallback at LBA 0 */
        com1_puts("[GPT] Checking for Legacy / MBR partition table at LBA 0...\r\n");
        if (parent_dev->read(parent_dev, 0, 1, sec_buf)) {
            if (sec_buf[510] == 0x55 && sec_buf[511] == 0xAA) {
                com1_puts("[MBR] Valid MBR signature (0xAA55) detected at LBA 0\r\n");
                for (int p = 0; p < 4; p++) {
                    uint8_t* entry = sec_buf + 0x1BE + (p * 16);
                    uint8_t type = entry[4];
                    if (type == 0 || type == 0xEE) continue;

                    uint32_t start_lba = *(uint32_t*)(entry + 8);
                    uint32_t sector_count = *(uint32_t*)(entry + 12);
                    if (sector_count == 0) continue;

                    int p_slot = s_gpt_telemetry.partition_count;
                    if (p_slot >= MAX_GPT_PARTS) break;
                    s_gpt_telemetry.partition_count++;

                    GPTPartitionInfo* pinfo = &s_gpt_telemetry.partitions[p_slot];
                    pinfo->part_index = p + 1;
                    pinfo->start_lba = start_lba;
                    pinfo->sector_count = sector_count;
                    pinfo->size_mb = (pinfo->sector_count * (uint64_t)sec_size) / (1024 * 1024);

                    if (type == 0x07) {
                        pinfo->is_windows_ntfs = true;
                        if (s_gpt_telemetry.windows_ntfs_part_idx < 0) {
                            s_gpt_telemetry.windows_ntfs_part_idx = p_slot;
                        }
                    } else if (type == 0x0B || type == 0x0C) {
                        pinfo->is_esp_fat32 = true;
                    }

                    com1_puts("      [MBR Part "); gpt_dbg_dec(p + 1);
                    com1_puts("] Type="); gpt_dbg_hex(type);
                    com1_puts(" StartLBA="); gpt_dbg_dec(start_lba);
                    com1_puts(" Count="); gpt_dbg_dec(sector_count);
                    com1_puts(" ("); gpt_dbg_dec(pinfo->size_mb / 1024); com1_puts(" GB)");
                    if (pinfo->is_windows_ntfs) com1_puts(" [NTFS/exFAT]");
                    if (pinfo->is_esp_fat32) com1_puts(" [FAT32]");
                    com1_puts("\r\n");

                    GPTPartContext* ctx = &s_part_contexts[p_slot];
                    ctx->parent = parent_dev;
                    ctx->start_lba = start_lba;
                    ctx->sector_count = sector_count;

                    BlockDevice* bdev = &s_part_bdevs[p_slot];
                    char* pname = s_part_names[p_slot];
                    strcpy(pname, parent_dev->name ? parent_dev->name : "disk");
                    int len = strlen(pname);
                    pname[len] = 'p';
                    pname[len + 1] = '0' + ((p + 1) % 10);
                    pname[len + 2] = '\0';

                    bdev->name = pname;
                    bdev->sector_size = sec_size;
                    bdev->sector_count = sector_count;
                    bdev->read_only = false;
                    bdev->driver_data = ctx;
                    bdev->read  = gpt_part_read;
                    bdev->write = gpt_part_write;
                    bdev->flush = gpt_part_flush;

                    int bd_id = block_device_register(bdev);
                    pinfo->bdev_id = bd_id;

                    if (pinfo->is_windows_ntfs && s_win_bdev_id < 0) {
                        s_win_bdev_id = bd_id;
                        s_gpt_telemetry.windows_ntfs_bdev_id = bd_id;
                    }

                    com1_puts("      -> Registered BlockDevice "); com1_puts(pname);
                    com1_puts(" (ID: "); gpt_dbg_dec(bd_id); com1_puts(")\r\n");
                }

                if (s_gpt_telemetry.partition_count > 0) {
                    s_gpt_telemetry.gpt_detected = true;
                    kfree(sec_buf);
                    return true;
                }
            }
        }
        kfree(sec_buf);
        return false;
    }

    s_gpt_telemetry.gpt_detected = true;
    uint64_t entry_lba = gpt_hdr->partition_entry_lba ? gpt_hdr->partition_entry_lba : 2;
    uint32_t entry_count = gpt_hdr->num_partition_entries;
    uint32_t entry_size = gpt_hdr->size_of_partition_entry ? gpt_hdr->size_of_partition_entry : 128;

    if (entry_count > 128) entry_count = 128;

    com1_puts("[GPT] Found Valid GPT Header!\r\n");
    com1_puts("      Partition Entries LBA: "); gpt_dbg_dec(entry_lba); com1_puts("\r\n");
    com1_puts("      Total Partitions:      "); gpt_dbg_dec(entry_count); com1_puts("\r\n");

    /* STEP 2: Read Partition Entries (each sector holds sec_size / entry_size entries) */
    uint32_t entries_per_sector = sec_size / entry_size;
    uint32_t sectors_to_read = (entry_count + entries_per_sector - 1) / entries_per_sector;
    uint32_t part_idx = 0;

    for (uint32_t s = 0; s < sectors_to_read && part_idx < entry_count; s++) {
        if (!parent_dev->read(parent_dev, entry_lba + s, 1, sec_buf)) {
            com1_puts("[GPT] Error reading partition entry sector ");
            gpt_dbg_dec(entry_lba + s); com1_puts("\r\n");
            break;
        }

        for (uint32_t e = 0; e < entries_per_sector && part_idx < entry_count; e++, part_idx++) {
            gpt_entry_t* entry = (gpt_entry_t*)(sec_buf + e * entry_size);

            /* Check if unused entry (type GUID is all zeros) */
            bool is_empty = true;
            for (int b = 0; b < 16; b++) {
                if (entry->type_guid[b] != 0) { is_empty = false; break; }
            }
            if (is_empty) continue;

            if (s_gpt_telemetry.partition_count >= MAX_GPT_PARTS) break;

            int p_slot = s_gpt_telemetry.partition_count++;
            GPTPartitionInfo* pinfo = &s_gpt_telemetry.partitions[p_slot];
            pinfo->part_index = part_idx + 1;
            pinfo->start_lba = entry->starting_lba;
            pinfo->sector_count = (entry->ending_lba >= entry->starting_lba) ?
                                  (entry->ending_lba - entry->starting_lba + 1) : 0;
            pinfo->size_mb = (pinfo->sector_count * (uint64_t)sec_size) / (1024 * 1024);

            /* Convert UTF-16LE partition name to ASCII */
            for (int n = 0; n < 36; n++) {
                pinfo->name[n] = (char)(entry->partition_name[n] & 0x7F);
                if (pinfo->name[n] == '\0') break;
            }
            pinfo->name[36] = '\0';

            /* Check GUID type */
            if (memcmp(entry->type_guid, GUID_BASIC_DATA, 16) == 0) {
                pinfo->is_windows_ntfs = true;
                if (s_gpt_telemetry.windows_ntfs_part_idx < 0) {
                    s_gpt_telemetry.windows_ntfs_part_idx = p_slot;
                }
            } else if (memcmp(entry->type_guid, GUID_EFI_SYSTEM, 16) == 0) {
                pinfo->is_esp_fat32 = true;
            }

            com1_puts("[GPT] Partition "); gpt_dbg_dec(pinfo->part_index);
            com1_puts(": StartLBA="); gpt_dbg_dec(pinfo->start_lba);
            com1_puts(" Count="); gpt_dbg_dec(pinfo->sector_count);
            com1_puts(" ("); gpt_dbg_dec(pinfo->size_mb / 1024); com1_puts(" GB)");
            if (pinfo->is_windows_ntfs) com1_puts(" [Microsoft Basic Data / NTFS]");
            if (pinfo->is_esp_fat32) com1_puts(" [EFI System Partition]");
            com1_puts("\r\n");

            /* STEP 3: Register Partition BlockDevice */
            GPTPartContext* ctx = &s_part_contexts[p_slot];
            ctx->parent = parent_dev;
            ctx->start_lba = pinfo->start_lba;
            ctx->sector_count = pinfo->sector_count;

            BlockDevice* bdev = &s_part_bdevs[p_slot];
            char* pname = s_part_names[p_slot];
            strcpy(pname, parent_dev->name ? parent_dev->name : "disk");
            int len = strlen(pname);
            pname[len] = 'p';
            pname[len + 1] = '0' + (pinfo->part_index % 10);
            pname[len + 2] = '\0';

            bdev->name = pname;
            bdev->sector_size = sec_size;
            bdev->sector_count = pinfo->sector_count;
            bdev->read_only = false;
            bdev->driver_data = ctx;
            bdev->read  = gpt_part_read;
            bdev->write = gpt_part_write;
            bdev->flush = gpt_part_flush;

            int bd_id = block_device_register(bdev);
            pinfo->bdev_id = bd_id;

            if (pinfo->is_windows_ntfs && s_win_bdev_id < 0) {
                s_win_bdev_id = bd_id;
                s_gpt_telemetry.windows_ntfs_bdev_id = bd_id;
            }

            com1_puts("      -> Registered BlockDevice "); com1_puts(pname);
            com1_puts(" (ID: "); gpt_dbg_dec(bd_id); com1_puts(")\r\n");
        }
    }

    kfree(sec_buf);
    return (s_gpt_telemetry.partition_count > 0);
}

const GPTTelemetry* gpt_get_telemetry(void) {
    return &s_gpt_telemetry;
}

BlockDevice* gpt_get_windows_ntfs_bdev(void) {
    if (s_win_bdev_id >= 0) {
        return block_device_get(s_win_bdev_id);
    }
    return NULL;
}
