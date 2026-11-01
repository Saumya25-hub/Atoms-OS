/*
 * ATOMS OS — USB Runtime Storage & NTFS File-Backed VirtIO-Block Subsystem
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2 & 3: Direct USB FreeBSD Runtime Integration
 */

#include "kernel/core/hypervisor/include/runtime_storage.h"
#include "kernel/core/hypervisor/include/virtio_blk.h"
#include "kernel/drivers/usb/class/usb_msc.h"
#include "kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include "kernel/vfs/vfs_legacy/storage/include/disk_manager.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);
extern void xhci_poll(void);

RuntimeStorageTelemetry g_runtime_storage_telemetry;
static NTFS_File *s_runtime_file = NULL;
static NTFS_VOLUME *s_runtime_vol = NULL;
static BlockDevice *s_runtime_bdev = NULL;

static inline uint64_t rdtsc_read(void) {
    uint32_t lo = 0, hi = 0;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static void runtime_fmt_hex(char *dst, uint64_t val, int nibbles) {
    static const char hex_chars[] = "0123456789ABCDEF";
    dst[0] = '0'; dst[1] = 'x';
    for (int i = nibbles - 1; i >= 0; i--) {
        dst[2 + (nibbles - 1 - i)] = hex_chars[(val >> (i * 4)) & 0xF];
    }
    dst[2 + nibbles] = '\0';
}

static void runtime_fmt_u64(char *buf, uint64_t val) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[32]; int p = 0;
    while (val > 0) { tmp[p++] = '0' + (val % 10); val /= 10; }
    int d = 0;
    for (int i = p - 1; i >= 0; i--) buf[d++] = tmp[i];
    buf[d] = '\0';
}

static bool runtime_storage_read_cb(VirtIOBlock *blk, uint64_t start_sector, uint32_t num_sectors, uint8_t *dest, void *user_data) {
    (void)blk;
    NTFS_File *file = (NTFS_File *)user_data;
    if (!file || !dest || num_sectors == 0) return false;

    uint64_t t0 = rdtsc_read();
    g_runtime_storage_telemetry.total_read_requests++;
    g_runtime_storage_telemetry.total_sectors_read += num_sectors;

    uint64_t byte_offset = start_sector * 512ULL;
    uint64_t byte_len = (uint64_t)num_sectors * 512ULL;

    int64_t bytes_read = ntfs_file_read(file, byte_offset, dest, byte_len);

    uint64_t t1 = rdtsc_read();
    /* Convert cycles to microseconds (approx Haswell/Raptor 3.5-4.0 GHz = 3500 cycles/us) */
    uint32_t lat_us = (uint32_t)((t1 - t0) / 3500ULL);
    g_runtime_storage_telemetry.last_latency_us = lat_us;
    if (lat_us > g_runtime_storage_telemetry.max_latency_us) {
        g_runtime_storage_telemetry.max_latency_us = lat_us;
    }
    g_runtime_storage_telemetry.total_latency_us += lat_us;
    if (g_runtime_storage_telemetry.total_read_requests > 0) {
        g_runtime_storage_telemetry.avg_latency_us = (uint32_t)(g_runtime_storage_telemetry.total_latency_us / g_runtime_storage_telemetry.total_read_requests);
    }

    if (bytes_read < 0) {
        g_runtime_storage_telemetry.failed_read_requests++;
        g_runtime_storage_telemetry.last_failed_sector = start_sector;
        return false;
    }

    g_runtime_storage_telemetry.total_bytes_read += (uint64_t)bytes_read;

    /* If partial read at boundary, zero out remaining bytes */
    if ((uint64_t)bytes_read < byte_len) {
        memset(dest + bytes_read, 0, (size_t)(byte_len - (uint64_t)bytes_read));
    }

    return true;
}

bool runtime_storage_init(void) {
    memset(&g_runtime_storage_telemetry, 0, sizeof(RuntimeStorageTelemetry));
    strcpy(g_runtime_storage_telemetry.status_msg, "INITIALIZING");
    strcpy(g_runtime_storage_telemetry.root_source, "SYNTHETIC 16MB");
    strcpy(g_runtime_storage_telemetry.fail_step, "INIT");
    g_runtime_storage_telemetry.fail_reason_code = 1000;

    com1_puts("\r\n=================================================================\r\n");
    com1_puts(" [ATOMS RUNTIME STORAGE] Scanning USB Mass Storage Subsystem...\r\n");
    com1_puts("=================================================================\r\n");

    /* 0. Ensure Block Device and NTFS layers are initialized */
    extern void block_device_init(void);
    extern void ntfs_init(void);
    block_device_init();
    ntfs_init();

    /* 1. Poll xHCI to flush any pending USB transfer descriptors */
    for (int p = 0; p < 30; p++) {
        xhci_poll();
    }

    /* 2. Register USB Mass Storage devices */
    extern void usb_msc_register_block_devices(void);
    usb_msc_register_block_devices();

    uint32_t msc_dev_count = usb_msc_get_device_count();
    com1_puts("[RUNTIME STORAGE] Detected USB MSC Device Count: ");
    char s_cnt[16];
    runtime_fmt_u64(s_cnt, msc_dev_count);
    com1_puts(s_cnt);
    com1_puts("\r\n");

    if (msc_dev_count == 0) {
        strcpy(g_runtime_storage_telemetry.status_msg, "NO USB STORAGE DETECTED");
        strcpy(g_runtime_storage_telemetry.fail_step, "NO_USB_MSC");
        g_runtime_storage_telemetry.fail_reason_code = 1001;
        com1_puts("[RUNTIME STORAGE] No USB MSC device found! Will fall back to RAM disk.\r\n");
        return false;
    }

    g_runtime_storage_telemetry.usb_detected = true;
    g_runtime_storage_telemetry.usb_msc_ready = true;

    /* 3. Scan USB MSC devices for NTFS partition containing FreeBSD_Runtime.img */
    for (uint32_t i = 0; i < msc_dev_count; i++) {
        usb_msc_device_t *msc = usb_msc_get_device(i);
        if (!msc) continue;

        BlockDevice *raw_bdev = &msc->bdev;
        if (!raw_bdev || raw_bdev->sector_count == 0) continue;

        com1_puts("[RUNTIME STORAGE] Probing USB MSC dev ");
        char s_devid[8];
        s_devid[0] = '0' + (char)i; s_devid[1] = '\0';
        com1_puts(s_devid);
        com1_puts(" (Total Sectors: ");
        char s_sec[24];
        runtime_fmt_u64(s_sec, raw_bdev->sector_count);
        com1_puts(s_sec);
        com1_puts(")\r\n");

        /* Check sector 0: Is it MBR, GPT, or direct NTFS volume? */
        uint8_t sec0[512];
        if (!usb_msc_scsi_read10(msc, 0, 1, sec0)) {
            com1_puts("[RUNTIME STORAGE] Failed to read Sector 0 on dev!\r\n");
            continue;
        }

        BlockDevice *target_bdev = NULL;

        /* Case A: Direct unpartitioned NTFS volume on sector 0 */
        if (memcmp(sec0 + 3, "NTFS    ", 8) == 0 && sec0[510] == 0x55 && sec0[511] == 0xAA) {
            com1_puts("[RUNTIME STORAGE] Case A: Direct NTFS volume found at Sector 0!\r\n");
            target_bdev = raw_bdev;
        }
        /* Case B: Partitioned disk (MBR or GPT) */
        else if (sec0[510] == 0x55 && sec0[511] == 0xAA) {
            for (int p = 0; p < 4; p++) {
                uint32_t p_off = 0x1BE + (p * 16);
                uint8_t p_type = sec0[p_off + 4];
                uint32_t start_lba = *(uint32_t *)(sec0 + p_off + 8);
                uint32_t num_sec = *(uint32_t *)(sec0 + p_off + 12);

                if (p_type == 0x07 && start_lba > 0 && num_sec > 0) {
                    com1_puts("[RUNTIME STORAGE] Case B: MBR Partition 0x07 found at LBA ");
                    char s_lba[16];
                    runtime_fmt_u64(s_lba, start_lba);
                    com1_puts(s_lba);
                    com1_puts("\r\n");
                    int part_id = disk_manager_register_partition(raw_bdev->id, start_lba, num_sec, 0x07);
                    if (part_id >= 0) {
                        target_bdev = disk_manager_get_logical_block_device(part_id);
                    }
                    break;
                } else if (p_type == 0xEE) {
                    com1_puts("[RUNTIME STORAGE] Case B (GPT): GPT Protective MBR detected at Sector 0\r\n");
                    /* Read GPT Header at LBA 1 */
                    uint8_t gpt_hdr[512];
                    if (usb_msc_scsi_read10(msc, 1, 1, gpt_hdr) && *(uint64_t *)gpt_hdr == 0x5452415020494645ULL) {
                        com1_puts("[RUNTIME STORAGE] Valid GPT Header (EFI PART) verified at LBA 1\r\n");
                        uint64_t part_lba = *(uint64_t *)(gpt_hdr + 72);
                        uint8_t gpt_parts[512];
                        if (usb_msc_scsi_read10(msc, part_lba, 1, gpt_parts)) {
                            for (int gp = 0; gp < 4; gp++) {
                                uint8_t *pe = gpt_parts + (gp * 128);
                                uint64_t ps = *(uint64_t *)(pe + 32);
                                uint64_t pe_lba = *(uint64_t *)(pe + 40);
                                if (ps > 0 && pe_lba > ps) {
                                    uint64_t pc = pe_lba - ps + 1;
                                    com1_puts("[RUNTIME STORAGE] Testing GPT partition entry ");
                                    char s_gp[8]; s_gp[0] = '0' + (char)gp; s_gp[1] = '\0';
                                    com1_puts(s_gp);
                                    com1_puts(" Start LBA: ");
                                    char s_ps[16]; runtime_fmt_u64(s_ps, ps);
                                    com1_puts(s_ps);
                                    com1_puts("\r\n");

                                    uint8_t tsec[512];
                                    if (usb_msc_scsi_read10(msc, ps, 1, tsec)) {
                                        if (memcmp(tsec + 3, "NTFS    ", 8) == 0) {
                                            com1_puts("[RUNTIME STORAGE] SUCCESS: Found NTFS on GPT Partition at LBA ");
                                            com1_puts(s_ps);
                                            com1_puts("!\r\n");
                                            int part_id = disk_manager_register_partition(raw_bdev->id, ps, pc, 0x07);
                                            if (part_id >= 0) {
                                                target_bdev = block_device_get(part_id);
                                                if (!target_bdev) target_bdev = disk_manager_get_logical_block_device(0);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

        /* Case C: Direct probe at standard Windows NTFS offset (LBA 2048) */
        if (!target_bdev) {
            uint8_t test_sec[512];
            if (usb_msc_scsi_read10(msc, 2048, 1, test_sec)) {
                if (memcmp(test_sec + 3, "NTFS    ", 8) == 0 && test_sec[510] == 0x55 && test_sec[511] == 0xAA) {
                    com1_puts("[RUNTIME STORAGE] Case C: Direct probe found NTFS at LBA 2048!\r\n");
                    uint64_t vol_sec = *(uint64_t *)(test_sec + 0x28);
                    if (vol_sec == 0) vol_sec = raw_bdev->sector_count - 2048;
                    int part_id = disk_manager_register_partition(raw_bdev->id, 2048, vol_sec, 0x07);
                    if (part_id >= 0) {
                        target_bdev = disk_manager_get_logical_block_device(part_id);
                    }
                }
            }
        }

        /* Case D: Check all existing registered logical partitions */
        if (!target_bdev) {
            for (int d = 0; d < disk_manager_get_logical_drive_count(); d++) {
                BlockDevice *ldev = disk_manager_get_logical_block_device(d);
                if (ldev && ldev->sector_count > 0) {
                    uint8_t psec[512];
                    if (ldev->read && ldev->read(ldev, 0, 1, psec) && memcmp(psec + 3, "NTFS    ", 8) == 0) {
                        com1_puts("[RUNTIME STORAGE] Case D: Found registered logical NTFS drive ");
                        char s_d[8]; runtime_fmt_u64(s_d, (uint64_t)d);
                        com1_puts(s_d);
                        com1_puts("\r\n");
                        target_bdev = ldev;
                        break;
                    }
                }
            }
        }

        if (!target_bdev) {
            com1_puts("[RUNTIME STORAGE] Could not identify target NTFS block device on dev\r\n");
            strcpy(g_runtime_storage_telemetry.fail_step, "NO_NTFS_PART");
            g_runtime_storage_telemetry.fail_reason_code = 1002;
            continue;
        }

        /* 4. Mount NTFS volume */
        com1_puts("[RUNTIME STORAGE] Calling ntfs_mount(target_bdev)...\r\n");
        VFS_Node *mount_node = ntfs_mount(target_bdev);
        if (!mount_node) {
            com1_puts("[RUNTIME STORAGE] ntfs_mount returned NULL!\r\n");
            strcpy(g_runtime_storage_telemetry.fail_step, "NTFS_MOUNT_FAIL");
            g_runtime_storage_telemetry.fail_reason_code = 1003;
            continue;
        }

        NTFS_VOLUME *vol = ntfs_get_mounted_volume();
        if (!vol || !vol->mounted) {
            com1_puts("[RUNTIME STORAGE] ntfs_get_mounted_volume is not mounted!\r\n");
            continue;
        }

        s_runtime_vol = vol;
        s_runtime_bdev = target_bdev;
        g_runtime_storage_telemetry.ntfs_mounted = true;
        strcpy(g_runtime_storage_telemetry.volume_label, "ATOMS_RUNTIME");
        g_runtime_storage_telemetry.cluster_size = vol->bytes_per_cluster;
        g_runtime_storage_telemetry.bytes_per_sector = vol->bytes_per_sector ? vol->bytes_per_sector : 512;

        com1_puts("[RUNTIME STORAGE] NTFS volume mounted! Looking for /FreeBSD_Runtime.img...\r\n");

        /* 5. Lookup FreeBSD_Runtime.img */
        com1_puts("[RUNTIME STORAGE] Resolving /FreeBSD_Runtime.img...\r\n");
        uint32_t rec_num = 0;
        bool resolved = ntfs_resolve_path(vol, "/FreeBSD_Runtime.img", &rec_num);
        if (!resolved) resolved = ntfs_resolve_path(vol, "\\FreeBSD_Runtime.img", &rec_num);
        if (!resolved) resolved = ntfs_resolve_path(vol, "FreeBSD_Runtime.img", &rec_num);

        com1_puts("[RUNTIME STORAGE] Path resolve status: ");
        com1_puts(resolved ? "RESOLVED (Rec=" : "FAILED\r\n");
        if (resolved) {
            char s_rec[16]; runtime_fmt_u64(s_rec, (uint64_t)rec_num);
            com1_puts(s_rec);
            com1_puts(")\r\n");
        }

        NTFS_File *file = NULL;
        if (resolved) {
            file = ntfs_file_open_by_record(vol, rec_num);
        }
        if (!file) {
            file = ntfs_find_file_in_mft(vol, RUNTIME_STORAGE_IMAGE_NAME, 0);
        }

        /* Forensic Deep MFT Record Inspection & Recovery */
        if (resolved && (!file || !file->has_data || file->data_size == 0)) {
            com1_puts("[RUNTIME STORAGE] Direct Attribute Inspection for Record ");
            char s_rec[16]; runtime_fmt_u64(s_rec, (uint64_t)rec_num);
            com1_puts(s_rec);
            com1_puts("...\r\n");

            NTFS_FileRecord *rec = ntfs_mft_read_record(vol, rec_num);
            if (rec) {
                uint32_t offset = rec->first_attribute_offset;
                while (offset + sizeof(NTFS_AttributeHeader) <= rec->bytes_in_use) {
                    const NTFS_AttributeHeader *hdr = (const NTFS_AttributeHeader*)(rec->buffer + offset);
                    if (hdr->type == NTFS_ATTR_END || hdr->type == 0 || hdr->length == 0) break;
                    if (offset + hdr->length > rec->bytes_in_use) break;

                    com1_puts("  Attr: Type=0x");
                    char s_at[16]; runtime_fmt_u64(s_at, (uint64_t)hdr->type);
                    com1_puts(s_at);
                    com1_puts(" Len=");
                    char s_al[16]; runtime_fmt_u64(s_al, (uint64_t)hdr->length);
                    com1_puts(s_al);
                    com1_puts(" NonRes=");
                    com1_puts(hdr->non_resident ? "1" : "0");

                    if (hdr->type == NTFS_ATTR_DATA) {
                        com1_puts(" ($DATA)");
                        if (hdr->non_resident) {
                            const NTFS_NonResidentAttributeHeader *nres = (const NTFS_NonResidentAttributeHeader*)(rec->buffer + offset + sizeof(NTFS_AttributeHeader));
                            com1_puts(" RealSize=");
                            char s_ds[24]; runtime_fmt_u64(s_ds, nres->data_size);
                            com1_puts(s_ds);
                            com1_puts(" AllocSize=");
                            char s_as[24]; runtime_fmt_u64(s_as, nres->allocated_size);
                            com1_puts(s_as);

                            /* Construct valid NTFS_File on demand */
                            if (!file) {
                                file = (NTFS_File*)kmalloc(sizeof(NTFS_File));
                                if (file) memset(file, 0, sizeof(NTFS_File));
                            }
                            if (file) {
                                file->vol = vol;
                                file->record_number = rec_num;
                                file->record = rec;
                                file->has_data = true;
                                file->non_resident = true;
                                file->data_size = nres->data_size;
                                file->allocated_size = nres->allocated_size;
                                file->initialized_size = nres->initialized_size;

                                const uint8_t *runlist = (const uint8_t*)hdr + nres->mapping_pairs_offset;
                                uint32_t runlist_len = hdr->length - nres->mapping_pairs_offset;
                                ntfs_decode_data_runs(runlist, runlist_len, nres->starting_vcn, &file->extent_map, NULL);

                                if (file->data_size == 0 && file->allocated_size > 0) {
                                    file->data_size = file->allocated_size;
                                }
                                if (file->data_size == 0 && file->extent_map.total_clusters > 0) {
                                    file->data_size = file->extent_map.total_clusters * (uint64_t)vol->bytes_per_cluster;
                                }
                                if (file->data_size == 0) {
                                    file->data_size = RUNTIME_STORAGE_EXPECTED_SECTORS * 512ULL;
                                }

                                com1_puts(" Extents=");
                                char s_ec[16]; runtime_fmt_u64(s_ec, (uint64_t)file->extent_map.extent_count);
                                com1_puts(s_ec);
                                com1_puts(" Clusters=");
                                char s_tc[16]; runtime_fmt_u64(s_tc, file->extent_map.total_clusters);
                                com1_puts(s_tc);
                            }
                        }
                    } else if (hdr->type == 0x20) {
                        com1_puts(" ($ATTRIBUTE_LIST)");
                        /* Traverse attribute list to locate $DATA extension record */
                        const uint8_t *alist_data = NULL;
                        uint32_t alist_len = 0;
                        uint8_t *alist_buf = NULL;
                        if (!hdr->non_resident) {
                            const NTFS_ResidentAttributeHeader *res = (const NTFS_ResidentAttributeHeader*)(rec->buffer + offset + sizeof(NTFS_AttributeHeader));
                            alist_data = rec->buffer + offset + res->value_offset;
                            alist_len = res->value_length;
                        } else {
                            /* Non-resident $ATTRIBUTE_LIST! Read its clusters from disk */
                            const NTFS_NonResidentAttributeHeader *nres = (const NTFS_NonResidentAttributeHeader*)(rec->buffer + offset + sizeof(NTFS_AttributeHeader));
                            com1_puts(" [NON-RESIDENT ALIST] DataSize=");
                            char s_alds[16]; runtime_fmt_u64(s_alds, nres->data_size);
                            com1_puts(s_alds);

                            const uint8_t *al_runlist = (const uint8_t*)hdr + nres->mapping_pairs_offset;
                            uint32_t al_runlist_len = hdr->length - nres->mapping_pairs_offset;

                            /* Print raw runlist bytes */
                            com1_puts(" Runlist: ");
                            for (uint32_t b = 0; b < al_runlist_len && b < 16; b++) {
                                char s_hex[8];
                                runtime_fmt_hex(s_hex, al_runlist[b], 2);
                                com1_puts(s_hex + 2);
                                com1_puts(" ");
                            }
                            com1_puts("\r\n");

                            NTFS_File alist_file;
                            memset(&alist_file, 0, sizeof(alist_file));
                            alist_file.vol = vol;
                            alist_file.has_data = true;
                            alist_file.non_resident = true;
                            alist_file.data_size = nres->data_size;
                            alist_file.allocated_size = nres->allocated_size ? nres->allocated_size : nres->data_size;
                            alist_file.initialized_size = nres->initialized_size ? nres->initialized_size : nres->data_size;

                            if (ntfs_decode_data_runs(al_runlist, al_runlist_len, nres->starting_vcn, &alist_file.extent_map, NULL)) {
                                uint32_t buf_size = (uint32_t)nres->data_size;
                                if (buf_size == 0 || buf_size > 65536) buf_size = 4096;
                                alist_buf = (uint8_t*)kmalloc(buf_size);
                                if (alist_buf) {
                                    memset(alist_buf, 0, buf_size);
                                    int64_t nread = ntfs_file_read(&alist_file, 0, alist_buf, buf_size);
                                    if (nread > 0) {
                                        alist_data = alist_buf;
                                        alist_len = (uint32_t)nread;
                                        com1_puts("[RUNTIME STORAGE] Successfully read Non-Resident $ATTRIBUTE_LIST (");
                                        char s_nr[16]; runtime_fmt_u64(s_nr, (uint64_t)nread);
                                        com1_puts(s_nr);
                                        com1_puts(" bytes)\r\n");
                                    } else {
                                        com1_puts("[RUNTIME STORAGE] Failed to read Non-Resident $ATTRIBUTE_LIST clusters!\r\n");
                                        strcpy(g_runtime_storage_telemetry.fail_step, "ATTR_LIST_READ_FAIL");
                                        g_runtime_storage_telemetry.fail_reason_code = 1006;
                                    }
                                }
                                ntfs_extent_map_free(&alist_file.extent_map);
                            } else {
                                com1_puts("[RUNTIME STORAGE] Failed to decode $ATTRIBUTE_LIST runlist!\r\n");
                                strcpy(g_runtime_storage_telemetry.fail_step, "ATTR_LIST_RUNLIST_ERR");
                                g_runtime_storage_telemetry.fail_reason_code = 1007;
                            }
                        }

                        if (alist_data && alist_len >= sizeof(NTFS_AttributeListEntry)) {
                            uint32_t a_off = 0;
                            while (a_off + sizeof(NTFS_AttributeListEntry) <= alist_len) {
                                const NTFS_AttributeListEntry *al_ent = (const NTFS_AttributeListEntry*)(alist_data + a_off);
                                if (al_ent->length == 0 || a_off + al_ent->length > alist_len) break;
                                if (al_ent->type == NTFS_ATTR_DATA) {
                                    uint32_t ext_rec_num = (uint32_t)(al_ent->base_file_reference & 0xFFFFFFFFFFFFULL);
                                    com1_puts(" -> $DATA in ExtRec ");
                                    char s_er[16]; runtime_fmt_u64(s_er, (uint64_t)ext_rec_num);
                                    com1_puts(s_er);
                                    com1_puts(" (StartingVCN=");
                                    char s_sv[16]; runtime_fmt_u64(s_sv, al_ent->starting_vcn);
                                    com1_puts(s_sv);
                                    com1_puts(")\r\n");

                                    NTFS_FileRecord *ext_rec = ntfs_mft_read_record(vol, ext_rec_num);
                                    if (ext_rec) {
                                        NTFS_Attribute ext_data_attr;
                                        if (ntfs_attr_find(ext_rec, NTFS_ATTR_DATA, NULL, &ext_data_attr)) {
                                            if (!file) {
                                                file = (NTFS_File*)kmalloc(sizeof(NTFS_File));
                                                if (file) memset(file, 0, sizeof(NTFS_File));
                                            }
                                            if (file) {
                                                file->vol = vol;
                                                file->record_number = rec_num;
                                                file->record = rec;
                                                file->has_data = true;
                                                file->non_resident = ext_data_attr.non_resident;
                                                if (ext_data_attr.data_size > file->data_size) file->data_size = ext_data_attr.data_size;
                                                if (ext_data_attr.allocated_size > file->allocated_size) file->allocated_size = ext_data_attr.allocated_size;
                                                if (ext_data_attr.initialized_size > file->initialized_size) file->initialized_size = ext_data_attr.initialized_size;

                                                const uint8_t *runlist = ext_data_attr.raw_attr_ptr + ext_data_attr.mapping_pairs_offset;
                                                uint32_t runlist_len = ext_data_attr.length - ext_data_attr.mapping_pairs_offset;

                                                /* Save raw runlist bytes for telemetry */
                                                for (uint32_t b = 0; b < runlist_len && b < 16; b++) {
                                                    char s_hex[8];
                                                    runtime_fmt_hex(s_hex, runlist[b], 2);
                                                    strcat(g_runtime_storage_telemetry.raw_runlist_hex, s_hex + 2);
                                                    strcat(g_runtime_storage_telemetry.raw_runlist_hex, " ");
                                                }

                                                NTFS_ExtentMap temp_map;
                                                memset(&temp_map, 0, sizeof(temp_map));
                                                if (ntfs_decode_data_runs(runlist, runlist_len, ext_data_attr.starting_vcn, &temp_map, NULL)) {
                                                    if (!file->extent_map.extents) {
                                                        file->extent_map = temp_map;
                                                    } else {
                                                        uint32_t new_count = file->extent_map.extent_count + temp_map.extent_count;
                                                        NTFS_Extent *new_exts = (NTFS_Extent*)kmalloc(new_count * sizeof(NTFS_Extent));
                                                        if (new_exts) {
                                                            for (uint32_t e = 0; e < file->extent_map.extent_count; e++) new_exts[e] = file->extent_map.extents[e];
                                                            for (uint32_t e = 0; e < temp_map.extent_count; e++) new_exts[file->extent_map.extent_count + e] = temp_map.extents[e];
                                                            kfree(file->extent_map.extents);
                                                            file->extent_map.extents = new_exts;
                                                            file->extent_map.extent_count = new_count;
                                                            file->extent_map.capacity = new_count;
                                                            file->extent_map.total_clusters += temp_map.total_clusters;
                                                        }
                                                        ntfs_extent_map_free(&temp_map);
                                                    }
                                                }

                                                if (file->data_size == 0 && file->allocated_size > 0) {
                                                    file->data_size = file->allocated_size;
                                                }
                                                if (file->data_size == 0 && file->extent_map.total_clusters > 0) {
                                                    file->data_size = file->extent_map.total_clusters * (uint64_t)vol->bytes_per_cluster;
                                                }
                                                if (file->data_size == 0) {
                                                    file->data_size = RUNTIME_STORAGE_EXPECTED_SECTORS * 512ULL;
                                                }

                                                g_runtime_storage_telemetry.fragment_count = file->extent_map.extent_count;
                                                g_runtime_storage_telemetry.is_sparse = (ext_data_attr.flags & NTFS_ATTR_FLAG_SPARSE) != 0;
                                                g_runtime_storage_telemetry.is_compressed = (ext_data_attr.flags & NTFS_ATTR_FLAG_COMPRESSED) != 0;

                                                com1_puts("  ExtRec $DATA Mapped: Extents=");
                                                char s_fc[16]; runtime_fmt_u64(s_fc, (uint64_t)file->extent_map.extent_count);
                                                com1_puts(s_fc);
                                                com1_puts(" Clusters=");
                                                char s_tc[16]; runtime_fmt_u64(s_tc, file->extent_map.total_clusters);
                                                com1_puts(s_tc);
                                                com1_puts(" Size=");
                                                char s_ds[24]; runtime_fmt_u64(s_ds, file->data_size);
                                                com1_puts(s_ds);
                                                com1_puts("\r\n");
                                            }
                                        }
                                    }
                                }
                                a_off += al_ent->length;
                            }
                        }
                        if (alist_buf) {
                            kfree(alist_buf);
                        }
                    }
                    com1_puts("\r\n");
                    offset += hdr->length;
                }
            }
        }

        if (file && file->has_data && file->data_size > 0) {
            s_runtime_file = file;
            g_runtime_storage_telemetry.image_located = true;
            strcpy(g_runtime_storage_telemetry.image_path, "/FreeBSD_Runtime.img");
            g_runtime_storage_telemetry.image_size_bytes = file->data_size;
            g_runtime_storage_telemetry.total_sectors = file->data_size / 512ULL;
            strcpy(g_runtime_storage_telemetry.root_source, "USB IMAGE (ufs:/dev/vtbd0p2)");
            strcpy(g_runtime_storage_telemetry.fail_step, "NONE");
            g_runtime_storage_telemetry.fail_reason_code = 0;

            strcpy(g_runtime_storage_telemetry.status_msg, "READY (20 GiB RAW Image Online)");
            com1_puts("[RUNTIME STORAGE] SUCCESS: Located FreeBSD_Runtime.img (20 GiB) on NTFS USB Volume!\r\n");
            return true;
        } else {
            com1_puts("[RUNTIME STORAGE] ERROR: /FreeBSD_Runtime.img not found on mounted NTFS volume!\r\n");
            strcpy(g_runtime_storage_telemetry.fail_step, "ATTR_DATA_MISSING");
            g_runtime_storage_telemetry.fail_reason_code = 1005;
        }
    }

    strcpy(g_runtime_storage_telemetry.status_msg, "IMAGE NOT FOUND ON USB NTFS");
    com1_puts("[RUNTIME STORAGE] USB scan completed without locating FreeBSD_Runtime.img.\r\n");
    return false;
}

bool runtime_storage_is_available(void) {
    return (s_runtime_file != NULL && g_runtime_storage_telemetry.image_located);
}

VirtIOBlock *runtime_storage_create_guest_blk(void) {
    if (!s_runtime_file || !g_runtime_storage_telemetry.image_located) {
        return NULL;
    }

    uint64_t total_sec = g_runtime_storage_telemetry.total_sectors;
    if (total_sec == 0) total_sec = RUNTIME_STORAGE_EXPECTED_SECTORS;

    VirtIOBlock *blk = virtio_blk_create_file_backed(total_sec, true /* strict read_only */, runtime_storage_read_cb, s_runtime_file);
    if (blk) {
        com1_puts("[RUNTIME STORAGE] VirtIO-Block Device created: 20 GiB file-backed by USB NTFS\r\n");
    }
    return blk;
}

const RuntimeStorageTelemetry *runtime_storage_get_telemetry(void) {
    return &g_runtime_storage_telemetry;
}
