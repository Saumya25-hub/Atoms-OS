#include "kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

// Forward declaration of filesystem driver structure
static VFS_Node* ntfs_mount_cb(BlockDevice* device);

// ---------------------------------------------------------------------------
// Sector Read Helper with Bounded Partition Checks
// ---------------------------------------------------------------------------
static bool ntfs_read_sector(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    if (!dev || !buffer) return false;
    if (dev->sector_count > 0 && (lba + count > dev->sector_count)) return false;
    if (dev->read) {
        return dev->read(dev, lba, count, buffer);
    }
    return block_device_read(dev->id, lba, count, buffer);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Phase 4 & 7: Cache Subsystems (Sector, MFT, Path Cache & Telemetry)
// ---------------------------------------------------------------------------
void ntfs_cache_init(NTFS_ReadCache* cache) {
    if (!cache) return;
    for (int i = 0; i < NTFS_CACHE_SIZE; i++) {
        cache->entries[i].valid = false;
        cache->entries[i].lba = 0;
        cache->entries[i].access_count = 0;
    }
    cache->hits = 0;
    cache->misses = 0;
    cache->evictions = 0;
}

void ntfs_cache_flush(NTFS_ReadCache* cache) {
    if (!cache) return;
    for (int i = 0; i < NTFS_CACHE_SIZE; i++) {
        cache->entries[i].valid = false;
    }
}

bool ntfs_read_sector_cached(NTFS_VOLUME* vol, uint64_t lba, void* buffer) {
    if (!vol || !buffer) return false;

    NTFS_ReadCache* cache = &vol->cache;

    // Check hit in cache
    for (int i = 0; i < NTFS_CACHE_SIZE; i++) {
        if (cache->entries[i].valid && cache->entries[i].lba == lba) {
            cache->hits++;
            vol->stats.sector_cache_hits++;
            cache->entries[i].access_count++;
            uint8_t* dst = (uint8_t*)buffer;
            for (int b = 0; b < 512; b++) dst[b] = cache->entries[i].data[b];
            return true;
        }
    }

    // Cache Miss
    cache->misses++;
    vol->stats.sector_cache_misses++;

    vol->stats.device_reads++;
    vol->stats.device_sectors_read++;
    if (!ntfs_read_sector(vol->device, lba, 1, buffer)) return false;

    // Find entry to replace
    int target_idx = -1;
    uint32_t min_access = 0xFFFFFFFF;

    for (int i = 0; i < NTFS_CACHE_SIZE; i++) {
        if (!cache->entries[i].valid) {
            target_idx = i;
            break;
        }
        if (cache->entries[i].access_count < min_access) {
            min_access = cache->entries[i].access_count;
            target_idx = i;
        }
    }

    if (target_idx >= 0) {
        if (cache->entries[target_idx].valid) {
            cache->evictions++;
            vol->stats.sector_cache_evictions++;
        }
        cache->entries[target_idx].lba = lba;
        cache->entries[target_idx].valid = true;
        cache->entries[target_idx].access_count = 1;
        uint8_t* src = (uint8_t*)buffer;
        for (int b = 0; b < 512; b++) cache->entries[target_idx].data[b] = src[b];
    }

    return true;
}

void ntfs_mft_cache_init(NTFS_MFTCache* cache) {
    if (!cache) return;
    for (int i = 0; i < NTFS_MFT_CACHE_SIZE; i++) {
        cache->entries[i].valid = false;
        cache->entries[i].record_number = 0;
        cache->entries[i].record_buffer = NULL;
        cache->entries[i].access_count = 0;
    }
    cache->hits = 0;
    cache->misses = 0;
    cache->evictions = 0;
}

void ntfs_mft_cache_flush(NTFS_MFTCache* cache) {
    if (!cache) return;
    for (int i = 0; i < NTFS_MFT_CACHE_SIZE; i++) {
        if (cache->entries[i].valid && cache->entries[i].record_buffer) {
            kfree(cache->entries[i].record_buffer);
            cache->entries[i].record_buffer = NULL;
        }
        cache->entries[i].valid = false;
    }
}

void ntfs_path_cache_init(NTFS_PathCache* cache) {
    if (!cache) return;
    for (int i = 0; i < NTFS_PATH_CACHE_SIZE; i++) {
        cache->entries[i].valid = false;
        cache->entries[i].path[0] = '\0';
        cache->entries[i].record_number = 0;
        cache->entries[i].access_count = 0;
    }
    cache->hits = 0;
    cache->misses = 0;
    cache->evictions = 0;
}

void ntfs_path_cache_flush(NTFS_PathCache* cache) {
    if (!cache) return;
    for (int i = 0; i < NTFS_PATH_CACHE_SIZE; i++) {
        cache->entries[i].valid = false;
    }
}

void ntfs_dump_performance_stats(const NTFS_VOLUME* vol) {
    if (!vol) return;
    display_print("\n=========================================\n");
    display_print(" [NTFS PHASE 7 PERFORMANCE DIAGNOSTICS]\n");
    display_print("-----------------------------------------\n");
    display_print(" Physical Device Reads : "); display_print_dec(vol->stats.device_reads); display_print("\n");
    display_print(" Physical Sectors Read : "); display_print_dec(vol->stats.device_sectors_read); display_print("\n");
    display_print(" Sector Cache Hits     : "); display_print_dec(vol->cache.hits); display_print("\n");
    display_print(" Sector Cache Misses   : "); display_print_dec(vol->cache.misses); display_print("\n");
    display_print(" Sector Cache Evict    : "); display_print_dec(vol->cache.evictions); display_print("\n");
    display_print(" MFT Cache Hits        : "); display_print_dec(vol->mft_cache.hits); display_print("\n");
    display_print(" MFT Cache Misses      : "); display_print_dec(vol->mft_cache.misses); display_print("\n");
    display_print(" MFT Cache Evictions   : "); display_print_dec(vol->mft_cache.evictions); display_print("\n");
    display_print(" Path Cache Hits       : "); display_print_dec(vol->path_cache.hits); display_print("\n");
    display_print(" Path Cache Misses     : "); display_print_dec(vol->path_cache.misses); display_print("\n");
    display_print(" Path Cache Evictions  : "); display_print_dec(vol->path_cache.evictions); display_print("\n");
    display_print(" Coalesced Multi-Read  : "); display_print_dec(vol->stats.coalesced_reads); display_print("\n");
    display_print(" Read-Ahead Triggers   : "); display_print_dec(vol->stats.read_ahead_triggers); display_print("\n");
    display_print(" Prefetched Sectors    : "); display_print_dec(vol->stats.prefetched_sectors); display_print("\n");
    display_print(" Sparse Bytes Zeroed   : "); display_print_dec(vol->stats.sparse_bytes_synthesized); display_print("\n");
    display_print(" Bytes Returned to VFS : "); display_print_dec(vol->stats.bytes_returned_vfs); display_print("\n");
    display_print("=========================================\n");
}

// ---------------------------------------------------------------------------
// Helper: Decodes FILE Record Size or Index Buffer Size
// ---------------------------------------------------------------------------
bool ntfs_decode_record_size(int8_t encoded, uint32_t bytes_per_cluster, uint32_t bytes_per_sector, uint32_t* out_size) {
    if (!out_size) return false;

    if (encoded > 0) {
        uint64_t calc = (uint64_t)encoded * (uint64_t)bytes_per_cluster;
        if (calc > 65536 || calc < 256) return false;
        *out_size = (uint32_t)calc;
        return true;
    } else if (encoded < 0) {
        int abs_val = -encoded;
        if (abs_val < 1 || abs_val > 31) return false;
        uint32_t calc = 1U << abs_val;
        if (calc > 65536 || calc < 256) return false;
        *out_size = calc;
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// 1C: BPB & Boot Sector Validation Engine
// ---------------------------------------------------------------------------
bool ntfs_validate_bpb(const NTFS_BootSector* bpb, uint64_t device_sector_count, const char** out_err_reason) {
    if (!bpb) {
        if (out_err_reason) *out_err_reason = "Null BPB pointer";
        return false;
    }

    const char expected_oem[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
    for (int i = 0; i < 8; i++) {
        if (bpb->oem_id[i] != expected_oem[i]) {
            if (out_err_reason) *out_err_reason = "Invalid OEM Signature (Not 'NTFS    ')";
            return false;
        }
    }

    if (bpb->boot_sector_signature != 0xAA55) {
        if (out_err_reason) *out_err_reason = "Invalid Boot Sector End Signature (Not 0xAA55)";
        return false;
    }

    uint16_t bps = bpb->bytes_per_sector;
    if (bps != 512 && bps != 1024 && bps != 2048 && bps != 4096) {
        if (out_err_reason) *out_err_reason = "Unsupported Bytes Per Sector (Must be 512, 1024, 2048, or 4096)";
        return false;
    }

    uint8_t spc = bpb->sectors_per_cluster;
    if (spc != 1 && spc != 2 && spc != 4 && spc != 8 &&
        spc != 16 && spc != 32 && spc != 64 && spc != 128) {
        if (out_err_reason) *out_err_reason = "Invalid Sectors Per Cluster (Must be power of two in range 1..128)";
        return false;
    }

    uint64_t cluster_size_calc = (uint64_t)bps * (uint64_t)spc;
    if (cluster_size_calc > 65536) {
        if (out_err_reason) *out_err_reason = "Cluster size exceeds maximum allowable 64 KB";
        return false;
    }
    uint32_t bytes_per_cluster = (uint32_t)cluster_size_calc;

    if (bpb->total_sectors == 0) {
        if (out_err_reason) *out_err_reason = "Zero Total Sectors in Boot Sector";
        return false;
    }
    if (device_sector_count > 0 && bpb->total_sectors > device_sector_count) {
        if (out_err_reason) *out_err_reason = "Total sectors exceed underlying bounded storage partition capacity";
        return false;
    }

    uint64_t total_clusters = bpb->total_sectors / spc;
    if (total_clusters == 0) {
        if (out_err_reason) *out_err_reason = "Calculated total clusters is zero";
        return false;
    }

    if (bpb->mft_cluster >= total_clusters) {
        if (out_err_reason) *out_err_reason = "$MFT starting cluster outside total volume cluster bounds";
        return false;
    }

    if (bpb->mft_mirr_cluster >= total_clusters) {
        if (out_err_reason) *out_err_reason = "$MFTMirr starting cluster outside total volume cluster bounds";
        return false;
    }

    uint32_t file_rec_size = 0;
    if (!ntfs_decode_record_size(bpb->clusters_per_mft_record, bytes_per_cluster, bps, &file_rec_size)) {
        if (out_err_reason) *out_err_reason = "Invalid FILE record size encoding";
        return false;
    }

    uint32_t idx_buf_size = 0;
    if (!ntfs_decode_record_size(bpb->clusters_per_index_buffer, bytes_per_cluster, bps, &idx_buf_size)) {
        if (out_err_reason) *out_err_reason = "Invalid Index Buffer size encoding";
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// 1F: Forensic Diagnostic Logger
// ---------------------------------------------------------------------------
void ntfs_dump_diagnostics(const NTFS_VOLUME* vol, const char* stage, bool success, const char* failure_reason) {
    display_print("\n=========================================\n");
    display_print(" [NTFS PHASE 1 FORENSIC DIAGNOSTICS]\n");
    display_print("-----------------------------------------\n");
    display_print(" Stage              : "); display_print(stage); display_print("\n");
    display_print(" Status             : "); display_print(success ? "SUCCESS" : "FAILED"); display_print("\n");

    if (vol && vol->device) {
        display_print(" Device Name        : "); display_print(vol->device->name ? vol->device->name : "N/A"); display_print("\n");
        display_print(" Device ID          : "); display_print_dec(vol->device->id); display_print("\n");
    }

    if (vol && vol->mounted) {
        char oem_str[9];
        for (int i = 0; i < 8; i++) oem_str[i] = vol->bpb.oem_id[i];
        oem_str[8] = '\0';

        display_print(" OEM Identifier     : "); display_print(oem_str); display_print("\n");
        display_print(" Bytes / Sector     : "); display_print_dec(vol->bytes_per_sector); display_print("\n");
        display_print(" Sectors / Cluster  : "); display_print_dec(vol->sectors_per_cluster); display_print("\n");
        display_print(" Bytes / Cluster    : "); display_print_dec(vol->bytes_per_cluster); display_print("\n");
        display_print(" Total Sectors      : "); display_print_dec(vol->total_sectors); display_print("\n");
        display_print(" Total Clusters     : "); display_print_dec(vol->total_clusters); display_print("\n");
        display_print(" Volume Size (Bytes): "); display_print_dec(vol->volume_size_bytes); display_print("\n");
        display_print(" $MFT LCN           : "); display_print_dec(vol->mft_lcn); display_print("\n");
        display_print(" $MFT Byte Offset   : "); display_print_dec(vol->mft_byte_offset); display_print("\n");
        display_print(" $MFTMirr LCN       : "); display_print_dec(vol->mft_mirr_lcn); display_print("\n");
        display_print(" $MFTMirr Offset    : "); display_print_dec(vol->mft_mirr_byte_offset); display_print("\n");
        display_print(" FILE Record Size   : "); display_print_dec(vol->file_record_size); display_print("\n");
        display_print(" Index Buffer Size  : "); display_print_dec(vol->index_buffer_size); display_print("\n");
        display_print(" Serial Number      : "); display_print_hex(vol->volume_serial_number); display_print("\n");
    }

    if (!success && failure_reason) {
        display_print(" Failure Reason     : "); display_print(failure_reason); display_print("\n");
    }
    display_print("=========================================\n");
}

// ---------------------------------------------------------------------------
// 1E & Phase 3 Bootstrap Mount Foundation
// ---------------------------------------------------------------------------
VFS_Node* ntfs_mount(BlockDevice* device) {
    if (!device) {
        ntfs_dump_diagnostics(NULL, "STAGE 1: Device Access", false, "Null block device pointer");
        return NULL;
    }

    if (device->sector_size == 0) {
        ntfs_dump_diagnostics(NULL, "STAGE 1: Device Access", false, "Zero sector size on device");
        return NULL;
    }

    uint8_t* sector_buf = (uint8_t*)kmalloc(512);
    if (!sector_buf) {
        ntfs_dump_diagnostics(NULL, "STAGE 2: Boot-Sector Read", false, "Kernel heap allocation failure for sector buffer");
        return NULL;
    }

    if (!ntfs_read_sector(device, 0, 1, sector_buf)) {
        ntfs_dump_diagnostics(NULL, "STAGE 2: Boot-Sector Read", false, "Block device I/O error reading LBA 0");
        kfree(sector_buf);
        return NULL;
    }

    const NTFS_BootSector* raw_bpb = (const NTFS_BootSector*)sector_buf;
    const char expected_oem[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
    bool sig_match = true;
    for (int i = 0; i < 8; i++) {
        if (raw_bpb->oem_id[i] != expected_oem[i]) {
            sig_match = false;
            break;
        }
    }

    if (!sig_match || raw_bpb->boot_sector_signature != 0xAA55) {
        ntfs_dump_diagnostics(NULL, "STAGE 3: NTFS Detection", false, "Signature mismatch (Not an NTFS boot sector)");
        kfree(sector_buf);
        return NULL;
    }

    const char* err_reason = NULL;
    if (!ntfs_validate_bpb(raw_bpb, device->sector_count, &err_reason)) {
        ntfs_dump_diagnostics(NULL, "STAGE 5: BPB Validation", false, err_reason);
        kfree(sector_buf);
        return NULL;
    }

    NTFS_VOLUME* volume = (NTFS_VOLUME*)kmalloc(sizeof(NTFS_VOLUME));
    if (!volume) {
        ntfs_dump_diagnostics(NULL, "STAGE 6: Geometry Calculation", false, "Heap allocation failed for NTFS_VOLUME context");
        kfree(sector_buf);
        return NULL;
    }

    uint8_t* dst = (uint8_t*)&volume->bpb;
    const uint8_t* src = (const uint8_t*)raw_bpb;
    for (uint32_t i = 0; i < sizeof(NTFS_BootSector); i++) dst[i] = src[i];

    volume->device = device;
    volume->bytes_per_sector = raw_bpb->bytes_per_sector;
    volume->sectors_per_cluster = raw_bpb->sectors_per_cluster;
    volume->bytes_per_cluster = volume->bytes_per_sector * volume->sectors_per_cluster;
    volume->total_sectors = raw_bpb->total_sectors;
    volume->total_clusters = volume->total_sectors / volume->sectors_per_cluster;
    volume->volume_size_bytes = volume->total_sectors * (uint64_t)volume->bytes_per_sector;

    volume->mft_lcn = raw_bpb->mft_cluster;
    volume->mft_byte_offset = volume->mft_lcn * (uint64_t)volume->bytes_per_cluster;

    volume->mft_mirr_lcn = raw_bpb->mft_mirr_cluster;
    volume->mft_mirr_byte_offset = volume->mft_mirr_lcn * (uint64_t)volume->bytes_per_cluster;

    ntfs_decode_record_size(raw_bpb->clusters_per_mft_record, volume->bytes_per_cluster, volume->bytes_per_sector, &volume->file_record_size);
    ntfs_decode_record_size(raw_bpb->clusters_per_index_buffer, volume->bytes_per_cluster, volume->bytes_per_sector, &volume->index_buffer_size);

    volume->volume_serial_number = raw_bpb->volume_serial_number;
    volume->mft_extent_map.extent_count = 0;
    volume->mft_extent_map.extents = NULL;

    ntfs_cache_init(&volume->cache);
    ntfs_mft_cache_init(&volume->mft_cache);
    ntfs_path_cache_init(&volume->path_cache);

    volume->stats.device_reads = 0;
    volume->stats.device_sectors_read = 0;
    volume->stats.sector_cache_hits = 0;
    volume->stats.sector_cache_misses = 0;
    volume->stats.sector_cache_evictions = 0;
    volume->stats.mft_cache_hits = 0;
    volume->stats.mft_cache_misses = 0;
    volume->stats.mft_cache_evictions = 0;
    volume->stats.path_cache_hits = 0;
    volume->stats.path_cache_misses = 0;
    volume->stats.path_cache_evictions = 0;
    volume->stats.sparse_bytes_synthesized = 0;
    volume->stats.read_ahead_triggers = 0;
    volume->stats.prefetched_sectors = 0;
    volume->stats.coalesced_reads = 0;
    volume->stats.bytes_returned_vfs = 0;

    VFS_Node* root = (VFS_Node*)kmalloc(sizeof(VFS_Node));
    if (!root) {
        ntfs_dump_diagnostics(NULL, "STAGE 7: Mount Context Init", false, "Heap allocation failed for VFS_Node root");
        kfree(volume);
        kfree(sector_buf);
        return NULL;
    }

    strcpy(root->name, "/");
    root->type = VFS_MOUNTPOINT;
    root->size = 0;
    root->parent = NULL;
    root->private_data = volume;
    root->fs_driver = &ntfs_fs_driver;

    volume->mounted = true;

    ntfs_bootstrap_mft_extent_map(volume);

    ntfs_dump_diagnostics(volume, "STAGE 7: Complete Mount Lifecycle", true, NULL);

    kfree(sector_buf);
    return root;
}

static VFS_Node* ntfs_mount_cb(BlockDevice* device) {
    return ntfs_mount(device);
}

// ---------------------------------------------------------------------------
// 1E: Unmount Foundation
// ---------------------------------------------------------------------------
int ntfs_unmount(VFS_Node* mount_node) {
    if (!mount_node) return -1;

    NTFS_VOLUME* volume = (NTFS_VOLUME*)mount_node->private_data;
    if (volume) {
        volume->mounted = false;
        volume->device = NULL;
        ntfs_cache_flush(&volume->cache);
        ntfs_mft_cache_flush(&volume->mft_cache);
        ntfs_path_cache_flush(&volume->path_cache);
        ntfs_extent_map_free(&volume->mft_extent_map);
        kfree(volume);
        mount_node->private_data = NULL;
    }

    kfree(mount_node);
    display_print("[NTFS] Unmounted Volume cleanly.\n");
    return 0;
}

// ===========================================================================
// PHASE 2 — MFT CORE ENGINE IMPLEMENTATION
// ===========================================================================

bool ntfs_mft_apply_fixup(uint8_t* buffer, uint32_t record_size, uint32_t bytes_per_sector, const char** out_err) {
    if (!buffer || record_size < sizeof(NTFS_FileRecordHeader) || bytes_per_sector < 512) {
        if (out_err) *out_err = "Null buffer or invalid geometry parameters";
        return false;
    }

    const NTFS_FileRecordHeader* hdr = (const NTFS_FileRecordHeader*)buffer;

    uint32_t usa_offset = (uint32_t)hdr->usa_offset;
    uint32_t usa_count  = (uint32_t)hdr->usa_count;

    if (usa_offset < 0x24 || usa_count == 0) {
        if (out_err) *out_err = "Invalid USA offset or zero count";
        return false;
    }

    uint64_t usa_end_calc = (uint64_t)usa_offset + ((uint64_t)usa_count * 2ULL);
    if (usa_end_calc > (uint64_t)record_size) {
        if (out_err) *out_err = "USA array extends beyond FILE record boundaries";
        return false;
    }

    uint32_t num_protected_sectors = record_size / bytes_per_sector;
    if (num_protected_sectors == 0) {
        if (out_err) *out_err = "Record size smaller than sector size";
        return false;
    }

    if (usa_count < num_protected_sectors + 1) {
        if (out_err) *out_err = "USA count inconsistent with protected sector count";
        return false;
    }

    const uint16_t* usa = (const uint16_t*)(buffer + usa_offset);
    uint16_t expected_usn = usa[0];

    for (uint32_t i = 0; i < num_protected_sectors; i++) {
        uint32_t sector_trailer_offset = ((i + 1) * bytes_per_sector) - 2;
        uint16_t actual_usn = *((uint16_t*)(buffer + sector_trailer_offset));
        if (actual_usn != expected_usn) {
            if (out_err) *out_err = "Sector trailer USN mismatch (Corrupted / Torn multi-sector write)";
            return false;
        }
    }

    for (uint32_t i = 0; i < num_protected_sectors; i++) {
        uint32_t sector_trailer_offset = ((i + 1) * bytes_per_sector) - 2;
        uint16_t replacement_word = usa[i + 1];
        *((uint16_t*)(buffer + sector_trailer_offset)) = replacement_word;
    }

    return true;
}

bool ntfs_mft_validate_record(const uint8_t* buffer, uint32_t record_size, const char** out_err) {
    if (!buffer || record_size < sizeof(NTFS_FileRecordHeader)) {
        if (out_err) *out_err = "Null buffer or undersized record";
        return false;
    }

    const NTFS_FileRecordHeader* hdr = (const NTFS_FileRecordHeader*)buffer;

    if (hdr->magic[0] != 'F' || hdr->magic[1] != 'I' || hdr->magic[2] != 'L' || hdr->magic[3] != 'E') {
        if (out_err) *out_err = "Invalid FILE record signature (Not 'FILE')";
        return false;
    }

    uint32_t usa_end = (uint32_t)hdr->usa_offset + ((uint32_t)hdr->usa_count * 2);
    if ((uint32_t)hdr->first_attribute_offset < usa_end) {
        if (out_err) *out_err = "First attribute offset overlaps header or USA array";
        return false;
    }
    if ((uint32_t)hdr->first_attribute_offset > hdr->bytes_in_use) {
        if (out_err) *out_err = "First attribute offset exceeds bytes_in_use";
        return false;
    }

    if (hdr->bytes_in_use < sizeof(NTFS_FileRecordHeader)) {
        if (out_err) *out_err = "bytes_in_use smaller than FILE record header size";
        return false;
    }
    if (hdr->bytes_in_use > hdr->bytes_allocated) {
        if (out_err) *out_err = "bytes_in_use exceeds bytes_allocated";
        return false;
    }

    if (hdr->bytes_allocated > record_size) {
        if (out_err) *out_err = "bytes_allocated exceeds total record buffer size";
        return false;
    }

    return true;
}

void ntfs_mft_dump_diagnostics(const NTFS_FileRecord* record, const char* stage, bool success, const char* err_reason) {
    display_print("\n=========================================\n");
    display_print(" [NTFS PHASE 2 MFT FORENSIC DIAGNOSTICS]\n");
    display_print("-----------------------------------------\n");
    display_print(" Stage              : "); display_print(stage); display_print("\n");
    display_print(" Status             : "); display_print(success ? "SUCCESS" : "FAILED"); display_print("\n");

    if (record) {
        display_print(" Record Number      : "); display_print_dec(record->record_number); display_print("\n");
        display_print(" Record Source      : "); display_print(record->source == NTFS_RECORD_SRC_PRIMARY ? "PRIMARY $MFT" : "FALLBACK $MFTMirr"); display_print("\n");
        display_print(" LBA Byte Offset    : "); display_print_dec(record->lba_offset); display_print("\n");
        display_print(" Record Size        : "); display_print_dec(record->record_size); display_print("\n");
        display_print(" State              : ");
        if (record->state == NTFS_RECORD_STATE_RAW) display_print("RAW\n");
        else if (record->state == NTFS_RECORD_STATE_FIXUP_APPLIED) display_print("FIXUP_APPLIED\n");
        else display_print("VALIDATED\n");

        display_print(" USA Offset         : "); display_print_dec(record->usa_offset); display_print("\n");
        display_print(" USA Count          : "); display_print_dec(record->usa_count); display_print("\n");
        display_print(" LSN                : "); display_print_hex(record->lsn); display_print("\n");
        display_print(" Sequence Number    : "); display_print_dec(record->sequence_number); display_print("\n");
        display_print(" Hard Links         : "); display_print_dec(record->hard_link_count); display_print("\n");
        display_print(" 1st Attr Offset    : "); display_print_dec(record->first_attribute_offset); display_print("\n");
        display_print(" Flags              : "); display_print_hex(record->flags); display_print("\n");
        display_print(" Bytes In Use       : "); display_print_dec(record->bytes_in_use); display_print("\n");
        display_print(" Bytes Allocated    : "); display_print_dec(record->bytes_allocated); display_print("\n");
    }

    if (!success && err_reason) {
        display_print(" Failure Reason     : "); display_print(err_reason); display_print("\n");
    }
    display_print("=========================================\n");
}

NTFS_FileRecord* ntfs_mft_read_record(NTFS_VOLUME* vol, uint32_t record_number) {
    if (!vol || !vol->mounted || !vol->device) {
        ntfs_mft_dump_diagnostics(NULL, "2A: MFT Discovery", false, "Invalid/unmounted NTFS volume context");
        return NULL;
    }

    uint32_t record_size = vol->file_record_size;
    if (record_size < sizeof(NTFS_FileRecordHeader) || vol->bytes_per_sector == 0) {
        ntfs_mft_dump_diagnostics(NULL, "2A: MFT Discovery", false, "Invalid volume record geometry");
        return NULL;
    }

    // 7B: Check MFT Cache Hit
    for (int i = 0; i < NTFS_MFT_CACHE_SIZE; i++) {
        if (vol->mft_cache.entries[i].valid && vol->mft_cache.entries[i].record_number == record_number) {
            vol->mft_cache.hits++;
            vol->stats.mft_cache_hits++;
            vol->mft_cache.entries[i].access_count++;

            uint8_t* cached_raw = (uint8_t*)kmalloc(record_size);
            if (!cached_raw) return NULL;
            for (uint32_t b = 0; b < record_size; b++) {
                cached_raw[b] = vol->mft_cache.entries[i].record_buffer[b];
            }

            NTFS_FileRecord* rec = (NTFS_FileRecord*)kmalloc(sizeof(NTFS_FileRecord));
            if (!rec) { kfree(cached_raw); return NULL; }

            rec->state                  = NTFS_RECORD_STATE_VALIDATED;
            rec->source                 = NTFS_RECORD_SRC_PRIMARY;
            rec->record_number          = record_number;
            rec->lba_offset             = vol->mft_byte_offset + ((uint64_t)record_number * record_size);
            rec->record_size            = vol->mft_cache.entries[i].record_size;
            rec->usa_offset             = vol->mft_cache.entries[i].usa_offset;
            rec->usa_count              = vol->mft_cache.entries[i].usa_count;
            rec->lsn                    = vol->mft_cache.entries[i].lsn;
            rec->sequence_number        = vol->mft_cache.entries[i].sequence_number;
            rec->hard_link_count        = vol->mft_cache.entries[i].hard_link_count;
            rec->first_attribute_offset = vol->mft_cache.entries[i].first_attribute_offset;
            rec->flags                  = vol->mft_cache.entries[i].flags;
            rec->bytes_in_use           = vol->mft_cache.entries[i].bytes_in_use;
            rec->bytes_allocated        = vol->mft_cache.entries[i].bytes_allocated;
            rec->base_file_record       = vol->mft_cache.entries[i].base_file_record;
            rec->next_attribute_id      = vol->mft_cache.entries[i].next_attribute_id;
            rec->buffer                 = cached_raw;

            return rec;
        }
    }

    vol->mft_cache.misses++;
    vol->stats.mft_cache_misses++;

    uint32_t sectors_per_record = (record_size + vol->bytes_per_sector - 1) / vol->bytes_per_sector;

    uint64_t relative_byte_offset = (uint64_t)record_number * (uint64_t)record_size;
    if ((relative_byte_offset / record_size) != record_number) {
        ntfs_mft_dump_diagnostics(NULL, "2A: MFT Discovery", false, "Record number multiplication overflow");
        return NULL;
    }

    uint8_t* raw_buf = (uint8_t*)kmalloc(record_size);
    if (!raw_buf) {
        ntfs_mft_dump_diagnostics(NULL, "2A: MFT Discovery", false, "Heap allocation failed for record buffer");
        return NULL;
    }

    uint64_t primary_byte_offset = vol->mft_byte_offset + relative_byte_offset;
    uint64_t primary_lba = primary_byte_offset / vol->bytes_per_sector;
    const char* primary_err = NULL;
    bool primary_ok = false;

    if (primary_lba + sectors_per_record <= vol->device->sector_count) {
        if (ntfs_read_sector(vol->device, primary_lba, sectors_per_record, raw_buf)) {
            if (ntfs_mft_apply_fixup(raw_buf, record_size, vol->bytes_per_sector, &primary_err)) {
                if (ntfs_mft_validate_record(raw_buf, record_size, &primary_err)) {
                    primary_ok = true;
                }
            }
        } else {
            primary_err = "Primary $MFT block device read I/O error";
        }
    } else {
        primary_err = "Primary $MFT read LBA exceeds partition boundary";
    }

    if (primary_ok) {
        NTFS_FileRecord* rec = (NTFS_FileRecord*)kmalloc(sizeof(NTFS_FileRecord));
        if (!rec) {
            kfree(raw_buf);
            return NULL;
        }

        const NTFS_FileRecordHeader* hdr = (const NTFS_FileRecordHeader*)raw_buf;
        rec->state                  = NTFS_RECORD_STATE_VALIDATED;
        rec->source                 = NTFS_RECORD_SRC_PRIMARY;
        rec->record_number          = record_number;
        rec->lba_offset             = primary_byte_offset;
        rec->record_size            = record_size;
        rec->usa_offset             = hdr->usa_offset;
        rec->usa_count              = hdr->usa_count;
        rec->lsn                    = hdr->lsn;
        rec->sequence_number        = hdr->sequence_number;
        rec->hard_link_count        = hdr->hard_link_count;
        rec->first_attribute_offset = hdr->first_attribute_offset;
        rec->flags                  = hdr->flags;
        rec->bytes_in_use           = hdr->bytes_in_use;
        rec->bytes_allocated        = hdr->bytes_allocated;
        rec->base_file_record       = hdr->base_file_record;
        rec->next_attribute_id      = hdr->next_attribute_id;
        rec->buffer                 = raw_buf;

        // 7B: Store Clean Record Copy in MFT Cache
        int target_idx = -1;
        uint32_t min_access = 0xFFFFFFFF;
        for (int i = 0; i < NTFS_MFT_CACHE_SIZE; i++) {
            if (!vol->mft_cache.entries[i].valid) {
                target_idx = i;
                break;
            }
            if (vol->mft_cache.entries[i].access_count < min_access) {
                min_access = vol->mft_cache.entries[i].access_count;
                target_idx = i;
            }
        }
        if (target_idx >= 0) {
            if (vol->mft_cache.entries[target_idx].valid) {
                vol->mft_cache.evictions++;
                vol->stats.mft_cache_evictions++;
                if (vol->mft_cache.entries[target_idx].record_buffer) {
                    kfree(vol->mft_cache.entries[target_idx].record_buffer);
                }
            }
            uint8_t* store_buf = (uint8_t*)kmalloc(record_size);
            if (store_buf) {
                for (uint32_t b = 0; b < record_size; b++) store_buf[b] = raw_buf[b];
                vol->mft_cache.entries[target_idx].record_number = record_number;
                vol->mft_cache.entries[target_idx].record_buffer = store_buf;
                vol->mft_cache.entries[target_idx].record_size = record_size;
                vol->mft_cache.entries[target_idx].usa_offset = hdr->usa_offset;
                vol->mft_cache.entries[target_idx].usa_count = hdr->usa_count;
                vol->mft_cache.entries[target_idx].lsn = hdr->lsn;
                vol->mft_cache.entries[target_idx].sequence_number = hdr->sequence_number;
                vol->mft_cache.entries[target_idx].hard_link_count = hdr->hard_link_count;
                vol->mft_cache.entries[target_idx].first_attribute_offset = hdr->first_attribute_offset;
                vol->mft_cache.entries[target_idx].flags = hdr->flags;
                vol->mft_cache.entries[target_idx].bytes_in_use = hdr->bytes_in_use;
                vol->mft_cache.entries[target_idx].bytes_allocated = hdr->bytes_allocated;
                vol->mft_cache.entries[target_idx].base_file_record = hdr->base_file_record;
                vol->mft_cache.entries[target_idx].next_attribute_id = hdr->next_attribute_id;
                vol->mft_cache.entries[target_idx].valid = true;
                vol->mft_cache.entries[target_idx].access_count = 1;
            }
        }

        ntfs_mft_dump_diagnostics(rec, "2D: Primary MFT Read & Validation", true, NULL);
        return rec;
    }

    ntfs_mft_dump_diagnostics(NULL, "2E: Primary MFT Record Failure", false, primary_err);

    if (record_number < 4 && vol->mft_mirr_byte_offset > 0) {
        display_print("[NTFS] Primary MFT Record "); display_print_dec(record_number);
        display_print(" failed. Attempting controlled $MFTMirr fallback...\n");

        uint64_t mirror_byte_offset = vol->mft_mirr_byte_offset + relative_byte_offset;
        uint64_t mirror_lba = mirror_byte_offset / vol->bytes_per_sector;
        const char* mirror_err = NULL;
        bool mirror_ok = false;

        if (mirror_lba + sectors_per_record <= vol->device->sector_count) {
            if (ntfs_read_sector(vol->device, mirror_lba, sectors_per_record, raw_buf)) {
                if (ntfs_mft_apply_fixup(raw_buf, record_size, vol->bytes_per_sector, &mirror_err)) {
                    if (ntfs_mft_validate_record(raw_buf, record_size, &mirror_err)) {
                        mirror_ok = true;
                    }
                }
            } else {
                mirror_err = "Mirror $MFTMirr block device read I/O error";
            }
        } else {
            mirror_err = "Mirror $MFTMirr read LBA exceeds partition boundary";
        }

        if (mirror_ok) {
            NTFS_FileRecord* rec = (NTFS_FileRecord*)kmalloc(sizeof(NTFS_FileRecord));
            if (!rec) {
                kfree(raw_buf);
                return NULL;
            }

            const NTFS_FileRecordHeader* hdr = (const NTFS_FileRecordHeader*)raw_buf;
            rec->state                  = NTFS_RECORD_STATE_VALIDATED;
            rec->source                 = NTFS_RECORD_SRC_MIRROR;
            rec->record_number          = record_number;
            rec->lba_offset             = mirror_byte_offset;
            rec->record_size            = record_size;
            rec->usa_offset             = hdr->usa_offset;
            rec->usa_count              = hdr->usa_count;
            rec->lsn                    = hdr->lsn;
            rec->sequence_number        = hdr->sequence_number;
            rec->hard_link_count        = hdr->hard_link_count;
            rec->first_attribute_offset = hdr->first_attribute_offset;
            rec->flags                  = hdr->flags;
            rec->bytes_in_use           = hdr->bytes_in_use;
            rec->bytes_allocated        = hdr->bytes_allocated;
            rec->base_file_record       = hdr->base_file_record;
            rec->next_attribute_id      = hdr->next_attribute_id;
            rec->buffer                 = raw_buf;

            ntfs_mft_dump_diagnostics(rec, "2E: $MFTMirr Fallback Success", true, NULL);
            return rec;
        }

        ntfs_mft_dump_diagnostics(NULL, "2E: $MFTMirr Fallback Failure", false, mirror_err);
    }

    kfree(raw_buf);
    return NULL;
}

void ntfs_mft_free_record(NTFS_FileRecord* record) {
    if (record) {
        if (record->buffer) {
            kfree(record->buffer);
            record->buffer = NULL;
        }
        record->state = NTFS_RECORD_STATE_RAW;
        kfree(record);
    }
}

// ===========================================================================
// PHASE 3 — ATTRIBUTE ENGINE IMPLEMENTATION
// ===========================================================================

bool ntfs_attr_find(const NTFS_FileRecord* rec, uint32_t attr_type, const char* name, NTFS_Attribute* out_attr) {
    (void)name;
    if (!rec || rec->state != NTFS_RECORD_STATE_VALIDATED || !rec->buffer || !out_attr) return false;

    uint32_t offset = rec->first_attribute_offset;
    uint32_t max_iterations = 256;
    uint32_t iter = 0;

    while (offset + sizeof(NTFS_AttributeHeader) <= rec->bytes_in_use && iter++ < max_iterations) {
        const NTFS_AttributeHeader* hdr = (const NTFS_AttributeHeader*)(rec->buffer + offset);

        if (hdr->type == NTFS_ATTR_END || hdr->type == 0 || hdr->length == 0) break;
        if (offset + hdr->length > rec->bytes_in_use) break;

        if (hdr->type == attr_type) {
            out_attr->type         = hdr->type;
            out_attr->length       = hdr->length;
            out_attr->non_resident = (hdr->non_resident != 0);
            out_attr->flags        = hdr->flags;
            out_attr->attribute_id = hdr->attribute_id;
            out_attr->raw_attr_ptr = (const uint8_t*)hdr;

            if (!out_attr->non_resident) {
                if (hdr->length < sizeof(NTFS_AttributeHeader) + sizeof(NTFS_ResidentAttributeHeader)) return false;
                const NTFS_ResidentAttributeHeader* res = (const NTFS_ResidentAttributeHeader*)(rec->buffer + offset + sizeof(NTFS_AttributeHeader));
                out_attr->resident_value_length = res->value_length;
                out_attr->resident_value_offset = res->value_offset;

                if (out_attr->resident_value_offset + out_attr->resident_value_length > hdr->length) return false;
            } else {
                if (hdr->length < sizeof(NTFS_AttributeHeader) + sizeof(NTFS_NonResidentAttributeHeader)) return false;
                const NTFS_NonResidentAttributeHeader* nres = (const NTFS_NonResidentAttributeHeader*)(rec->buffer + offset + sizeof(NTFS_AttributeHeader));
                out_attr->starting_vcn         = nres->starting_vcn;
                out_attr->last_vcn             = nres->last_vcn;
                out_attr->mapping_pairs_offset = nres->mapping_pairs_offset;
                out_attr->allocated_size       = nres->allocated_size;
                out_attr->data_size            = nres->data_size;
                out_attr->initialized_size     = nres->initialized_size;

                if (out_attr->starting_vcn > out_attr->last_vcn && out_attr->data_size > 0) return false;
                if (out_attr->mapping_pairs_offset > hdr->length) return false;
            }

            return true;
        }

        offset += hdr->length;
    }

    return false;
}

bool ntfs_attr_get_resident_value(const NTFS_FileRecord* rec, const NTFS_Attribute* attr, const void** out_data, uint32_t* out_len) {
    if (!rec || !attr || !out_data || !out_len || attr->non_resident) return false;
    if (!attr->raw_attr_ptr || attr->raw_attr_ptr < rec->buffer) return false;
    if (attr->raw_attr_ptr + attr->length > rec->buffer + rec->bytes_in_use) return false;

    if (attr->resident_value_offset + attr->resident_value_length > attr->length) return false;

    *out_data = (const void*)(attr->raw_attr_ptr + attr->resident_value_offset);
    *out_len  = attr->resident_value_length;
    return true;
}

bool ntfs_decode_data_runs(const uint8_t* runlist, uint32_t runlist_len, uint64_t starting_vcn, NTFS_ExtentMap* out_map, const char** out_err) {
    if (!runlist || !out_map) {
        if (out_err) *out_err = "Null runlist or extent map pointer";
        return false;
    }

    out_map->extent_count = 0;
    out_map->capacity = 16;
    out_map->extents = (NTFS_Extent*)kmalloc(out_map->capacity * sizeof(NTFS_Extent));
    if (!out_map->extents) {
        if (out_err) *out_err = "Heap allocation failed for extent array";
        return false;
    }

    uint32_t pos = 0;
    uint64_t current_vcn = starting_vcn;
    int64_t  current_lcn = 0;

    while (pos < runlist_len) {
        uint8_t header = runlist[pos++];
        if (header == 0) break;

        uint8_t len_bytes = header & 0x0F;
        uint8_t off_bytes = (header >> 4) & 0x0F;

        if (len_bytes == 0 || len_bytes > 8 || off_bytes > 8 || (pos + len_bytes + off_bytes > runlist_len)) {
            if (out_err) *out_err = "Invalid run header byte or truncated runlist";
            ntfs_extent_map_free(out_map);
            return false;
        }

        uint64_t run_len = 0;
        for (uint8_t i = 0; i < len_bytes; i++) {
            run_len |= ((uint64_t)runlist[pos++]) << (i * 8);
        }
        if (run_len == 0) {
            if (out_err) *out_err = "Zero run length in mapping pairs";
            ntfs_extent_map_free(out_map);
            return false;
        }

        int64_t lcn_delta = 0;
        if (off_bytes > 0) {
            for (uint8_t i = 0; i < off_bytes; i++) {
                lcn_delta |= ((int64_t)runlist[pos++]) << (i * 8);
            }
            if (runlist[pos - 1] & 0x80) {
                lcn_delta |= (-1LL << (off_bytes * 8));
            }
        }

        NTFS_Extent ext;
        ext.vcn_start = current_vcn;
        ext.cluster_count = run_len;

        if (off_bytes == 0) {
            ext.is_sparse = true;
            ext.lcn_start = -1;
        } else {
            ext.is_sparse = false;
            current_lcn += lcn_delta;
            if (current_lcn < 0) {
                if (out_err) *out_err = "Negative accumulated LCN delta";
                ntfs_extent_map_free(out_map);
                return false;
            }
            ext.lcn_start = current_lcn;
        }

        current_vcn += run_len;

        if (out_map->extent_count >= out_map->capacity) {
            uint32_t new_cap = out_map->capacity * 2;
            NTFS_Extent* new_exts = (NTFS_Extent*)kmalloc(new_cap * sizeof(NTFS_Extent));
            if (!new_exts) {
                if (out_err) *out_err = "Extent array memory reallocation failure";
                ntfs_extent_map_free(out_map);
                return false;
            }
            for (uint32_t i = 0; i < out_map->extent_count; i++) new_exts[i] = out_map->extents[i];
            kfree(out_map->extents);
            out_map->extents = new_exts;
            out_map->capacity = new_cap;
        }

        out_map->extents[out_map->extent_count++] = ext;
    }

    out_map->total_clusters = current_vcn - starting_vcn;
    return true;
}

bool ntfs_extent_map_lookup(const NTFS_ExtentMap* map, uint64_t vcn, NTFS_Extent* out_extent) {
    if (!map || !map->extents || !out_extent) return false;

    for (uint32_t i = 0; i < map->extent_count; i++) {
        const NTFS_Extent* ext = &map->extents[i];
        if (vcn >= ext->vcn_start && vcn < (ext->vcn_start + ext->cluster_count)) {
            *out_extent = *ext;
            return true;
        }
    }

    return false;
}

void ntfs_extent_map_free(NTFS_ExtentMap* map) {
    if (map && map->extents) {
        kfree(map->extents);
        map->extents = NULL;
        map->extent_count = 0;
        map->capacity = 0;
    }
}

bool ntfs_bootstrap_mft_extent_map(NTFS_VOLUME* vol) {
    if (!vol || !vol->mounted) return false;

    NTFS_FileRecord* mft_rec = ntfs_mft_read_record(vol, 0);
    if (!mft_rec) return false;

    NTFS_Attribute data_attr;
    if (!ntfs_attr_find(mft_rec, NTFS_ATTR_DATA, NULL, &data_attr)) {
        ntfs_mft_free_record(mft_rec);
        return false;
    }

    if (!data_attr.non_resident) {
        vol->mft_extent_map.extent_count = 1;
        vol->mft_extent_map.capacity = 1;
        vol->mft_extent_map.extents = (NTFS_Extent*)kmalloc(sizeof(NTFS_Extent));
        if (vol->mft_extent_map.extents) {
            vol->mft_extent_map.extents[0].vcn_start = 0;
            vol->mft_extent_map.extents[0].cluster_count = (vol->file_record_size + vol->bytes_per_cluster - 1) / vol->bytes_per_cluster;
            vol->mft_extent_map.extents[0].lcn_start = vol->mft_lcn;
            vol->mft_extent_map.extents[0].is_sparse = false;
            vol->mft_extent_map.total_clusters = vol->mft_extent_map.extents[0].cluster_count;
        }
    } else {
        const uint8_t* runlist = data_attr.raw_attr_ptr + data_attr.mapping_pairs_offset;
        uint32_t runlist_len = data_attr.length - data_attr.mapping_pairs_offset;
        ntfs_decode_data_runs(runlist, runlist_len, data_attr.starting_vcn, &vol->mft_extent_map, NULL);
    }

    ntfs_mft_free_record(mft_rec);
    return true;
}

// ===========================================================================
// PHASE 4 — FILE READ ENGINE IMPLEMENTATION
// ===========================================================================

// 4A: File Open by MFT Record Number
NTFS_File* ntfs_file_open_by_record(NTFS_VOLUME* vol, uint32_t record_number) {
    if (!vol || !vol->mounted) return NULL;

    NTFS_FileRecord* rec = ntfs_mft_read_record(vol, record_number);
    if (!rec) return NULL;

    NTFS_File* file = (NTFS_File*)kmalloc(sizeof(NTFS_File));
    if (!file) {
        ntfs_mft_free_record(rec);
        return NULL;
    }

    file->vol = vol;
    file->record_number = record_number;
    file->record = rec;
    file->is_directory = (rec->flags & NTFS_FILE_DIRECTORY) != 0;
    file->extent_map.extents = NULL;
    file->extent_map.extent_count = 0;
    file->last_read_offset = 0;
    file->sequential_read_count = 0;

    NTFS_Attribute data_attr;
    bool has_data = ntfs_attr_find(rec, NTFS_ATTR_DATA, NULL, &data_attr);
    if (!has_data) {
        file->has_data = false;
        file->data_size = 0;
        file->allocated_size = 0;
        file->initialized_size = 0;
        file->non_resident = false;
        file->resident_data = NULL;
        file->resident_len = 0;
        file->is_compressed = false;
        file->is_encrypted = false;
        return file;
    }

    file->has_data = true;
    file->non_resident = data_attr.non_resident;
    file->is_compressed = (data_attr.flags & NTFS_ATTR_FLAG_COMPRESSED) != 0;
    file->is_encrypted = (data_attr.flags & NTFS_ATTR_FLAG_ENCRYPTED) != 0;

    if (!data_attr.non_resident) {
        // 4B: Resident Stream Initialization
        const void* val_ptr = NULL;
        uint32_t val_len = 0;
        if (ntfs_attr_get_resident_value(rec, &data_attr, &val_ptr, &val_len)) {
            file->resident_data = (const uint8_t*)val_ptr;
            file->resident_len = val_len;
            file->data_size = val_len;
            file->allocated_size = val_len;
            file->initialized_size = val_len;
        } else {
            file->has_data = false;
            file->data_size = 0;
        }
    } else {
        // 4C: Non-Resident Stream Initialization
        file->resident_data = NULL;
        file->resident_len = 0;
        file->data_size = data_attr.data_size;
        file->allocated_size = data_attr.allocated_size;
        file->initialized_size = data_attr.initialized_size;

        const uint8_t* runlist = data_attr.raw_attr_ptr + data_attr.mapping_pairs_offset;
        uint32_t runlist_len = data_attr.length - data_attr.mapping_pairs_offset;
        ntfs_decode_data_runs(runlist, runlist_len, data_attr.starting_vcn, &file->extent_map, NULL);
    }

    return file;
}

// 4A: File Context Cleanup
void ntfs_file_close(NTFS_File* file) {
    if (file) {
        if (file->extent_map.extents) {
            ntfs_extent_map_free(&file->extent_map);
        }
        if (file->record) {
            ntfs_mft_free_record(file->record);
            file->record = NULL;
        }
        kfree(file);
    }
}

// 4B, 4C, 4D, 4E: Offset, Range, Fragmented, & Sparse File Read Engine
int64_t ntfs_file_read(NTFS_File* file, uint64_t offset, void* buffer, uint64_t len) {
    if (!file || !buffer) return -1;
    if (!file->has_data || len == 0 || offset >= file->data_size) return 0; // EOF

    // Compressed / Encrypted streams return explicit unsupported error code (-1)
    if (file->is_compressed || file->is_encrypted) return -1;

    // Range clamping at logical EOF
    uint64_t bytes_to_read = len;
    if (offset + bytes_to_read > file->data_size) {
        bytes_to_read = file->data_size - offset;
    }
    if (offset + bytes_to_read < offset) return -1; // Overflow check

    // 7F: Track Bytes Returned to VFS
    if (file->vol) file->vol->stats.bytes_returned_vfs += bytes_to_read;

    // 7D: Sequential Read-Ahead Detection
    if (offset == file->last_read_offset && offset > 0) {
        file->sequential_read_count++;
    } else {
        file->sequential_read_count = 1;
    }
    file->last_read_offset = offset + bytes_to_read;

    if (file->sequential_read_count >= 2 && file->non_resident && !file->is_directory && file->vol) {
        uint64_t next_offset = offset + bytes_to_read;
        if (next_offset < file->initialized_size) {
            uint64_t next_vcn = next_offset / file->vol->bytes_per_cluster;
            NTFS_Extent next_ext;
            if (ntfs_extent_map_lookup(&file->extent_map, next_vcn, &next_ext) && !next_ext.is_sparse) {
                uint64_t next_phys_cluster = next_ext.lcn_start + (next_vcn - next_ext.vcn_start);
                uint64_t next_lba = (next_phys_cluster * file->vol->bytes_per_cluster) / 512;
                file->vol->stats.read_ahead_triggers++;
                uint8_t ra_buf[512];
                for (uint64_t p = 0; p < 8; p++) {
                    if (next_lba + p < file->vol->device->sector_count) {
                        if (ntfs_read_sector_cached(file->vol, next_lba + p, ra_buf)) {
                            file->vol->stats.prefetched_sectors++;
                        }
                    }
                }
            }
        }
    }

    // 4B: Resident File Read Path
    if (!file->non_resident) {
        if (!file->resident_data || offset + bytes_to_read > file->resident_len) return -1;
        uint8_t* dst = (uint8_t*)buffer;
        for (uint64_t i = 0; i < bytes_to_read; i++) {
            dst[i] = file->resident_data[offset + i];
        }
        return (int64_t)bytes_to_read;
    }

    // 4C, 4D, 4E, 7C: Non-Resident, Range, Fragmented, Sparse & Coalesced Read Path
    uint64_t bytes_read = 0;
    uint8_t* dst = (uint8_t*)buffer;
    uint8_t* sector_bounce = (uint8_t*)kmalloc(512);
    if (!sector_bounce) return -1;

    while (bytes_read < bytes_to_read) {
        uint64_t curr_offset = offset + bytes_read;
        uint64_t remaining_in_req = bytes_to_read - bytes_read;

        // Security Requirement: Initialized Size Protection (Uninitialized logical gap zeroing)
        if (curr_offset >= file->initialized_size) {
            for (uint64_t i = 0; i < remaining_in_req; i++) {
                dst[bytes_read + i] = 0;
            }
            bytes_read += remaining_in_req;
            break;
        }

        uint64_t vcn = curr_offset / file->vol->bytes_per_cluster;
        uint32_t intra_cluster_offset = (uint32_t)(curr_offset % file->vol->bytes_per_cluster);

        NTFS_Extent extent;
        if (!ntfs_extent_map_lookup(&file->extent_map, vcn, &extent)) break;

        uint64_t extent_end_byte = (extent.vcn_start + extent.cluster_count) * file->vol->bytes_per_cluster;
        uint64_t avail_in_extent = extent_end_byte - curr_offset;
        uint64_t chunk_size = remaining_in_req < avail_in_extent ? remaining_in_req : avail_in_extent;

        if (curr_offset + chunk_size > file->initialized_size) {
            chunk_size = file->initialized_size - curr_offset;
        }

        if (extent.is_sparse) {
            // 4E & 7F: Sparse Extent Zero Synthesis
            for (uint64_t i = 0; i < chunk_size; i++) {
                dst[bytes_read + i] = 0;
            }
            if (file->vol) file->vol->stats.sparse_bytes_synthesized += chunk_size;
        } else {
            // 4C & 7C: Physical Extent Read with Read Coalescing
            uint64_t cluster_offset_in_extent = vcn - extent.vcn_start;
            uint64_t phys_cluster = extent.lcn_start + cluster_offset_in_extent;
            uint64_t phys_byte_offset = (phys_cluster * file->vol->bytes_per_cluster) + intra_cluster_offset;

            uint64_t chunk_pos = 0;
            while (chunk_pos < chunk_size) {
                uint64_t byte_addr = phys_byte_offset + chunk_pos;
                uint64_t sec_lba = byte_addr / 512;
                uint32_t sec_off = (uint32_t)(byte_addr % 512);
                uint64_t rem_in_chunk = chunk_size - chunk_pos;

                // 7C: Multi-sector Read Coalescing for Aligned Contiguous Block Reads
                if (sec_off == 0 && rem_in_chunk >= 1024) {
                    uint32_t contig_secs = (uint32_t)(rem_in_chunk / 512);
                    if (sec_lba + contig_secs <= file->vol->device->sector_count) {
                        if (file->vol) file->vol->stats.coalesced_reads++;
                        for (uint32_t s = 0; s < contig_secs; s++) {
                            ntfs_read_sector_cached(file->vol, sec_lba + s, dst + bytes_read + chunk_pos + (s * 512));
                        }
                        chunk_pos += (uint64_t)contig_secs * 512;
                        continue;
                    }
                }

                uint32_t sec_avail = 512 - sec_off;
                uint32_t sec_chunk = rem_in_chunk < sec_avail ? (uint32_t)rem_in_chunk : sec_avail;

                if (sec_off == 0 && sec_chunk == 512) {
                    // Full sector cached read
                    ntfs_read_sector_cached(file->vol, sec_lba, dst + bytes_read + chunk_pos);
                } else {
                    // Sector bounce cached read for unaligned byte boundaries
                    if (ntfs_read_sector_cached(file->vol, sec_lba, sector_bounce)) {
                        for (uint32_t b = 0; b < sec_chunk; b++) {
                            dst[bytes_read + chunk_pos + b] = sector_bounce[sec_off + b];
                        }
                    } else {
                        break;
                    }
                }
                chunk_pos += sec_chunk;
            }
        }

        bytes_read += chunk_size;
    }

    kfree(sector_bounce);
    return (int64_t)bytes_read;
}

// ===========================================================================
// PHASE 5 — DIRECTORY & INDEX ENGINE IMPLEMENTATION
// ===========================================================================

static int ntfs_filename_cmp(const char* target, const uint16_t* u16_name, uint8_t u16_len) {
    uint32_t target_len = 0;
    while (target[target_len] != '\0') target_len++;

    uint32_t cmp_len = target_len < u16_len ? target_len : u16_len;

    for (uint32_t i = 0; i < cmp_len; i++) {
        char c1 = target[i];
        char c2 = (char)(u16_name[i] & 0x7F);

        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;

        if (c1 != c2) return (int)c1 - (int)c2;
    }

    if (target_len != u16_len) return (int)target_len - (int)u16_len;
    return 0;
}

static bool ntfs_indx_validate_and_fixup(uint8_t* buffer, uint32_t buffer_size, uint32_t bytes_per_sector) {
    if (!buffer || buffer_size < sizeof(NTFS_IndexBlockHeader)) return false;

    NTFS_IndexBlockHeader* hdr = (NTFS_IndexBlockHeader*)buffer;
    if (hdr->magic[0] != 'I' || hdr->magic[1] != 'N' || hdr->magic[2] != 'D' || hdr->magic[3] != 'X') return false;

    return ntfs_mft_apply_fixup(buffer, buffer_size, bytes_per_sector, NULL);
}

static bool ntfs_index_bitmap_is_allocated(NTFS_VOLUME* vol, const NTFS_FileRecord* dir_rec, uint64_t vcn_block_index) {
    (void)vol;
    if (!dir_rec) return true;

    NTFS_Attribute bmp_attr;
    if (!ntfs_attr_find(dir_rec, NTFS_ATTR_BITMAP, "$I30", &bmp_attr) &&
        !ntfs_attr_find(dir_rec, NTFS_ATTR_BITMAP, NULL, &bmp_attr)) {
        return true;
    }

    if (!bmp_attr.non_resident) {
        const void* val_ptr = NULL; uint32_t val_len = 0;
        if (ntfs_attr_get_resident_value(dir_rec, &bmp_attr, &val_ptr, &val_len)) {
            const uint8_t* bmp = (const uint8_t*)val_ptr;
            uint32_t byte_off = (uint32_t)(vcn_block_index / 8);
            uint8_t bit_off = (uint8_t)(vcn_block_index % 8);
            if (byte_off < val_len) {
                return (bmp[byte_off] & (1 << bit_off)) != 0;
            }
        }
    }
    return true;
}

// 5D: B+Tree Directory Lookup
bool ntfs_dir_lookup_entry(NTFS_VOLUME* vol, const NTFS_FileRecord* dir_rec, const char* name, uint64_t* out_file_ref) {
    if (!vol || !dir_rec || !name || !out_file_ref) return false;

    NTFS_Attribute root_attr;
    if (!ntfs_attr_find(dir_rec, NTFS_ATTR_INDEX_ROOT, "$I30", &root_attr) &&
        !ntfs_attr_find(dir_rec, NTFS_ATTR_INDEX_ROOT, NULL, &root_attr)) {
        return false;
    }

    const void* root_val = NULL; uint32_t root_len = 0;
    if (!ntfs_attr_get_resident_value(dir_rec, &root_attr, &root_val, &root_len)) return false;
    if (root_len < sizeof(NTFS_IndexRootHeader) + sizeof(NTFS_IndexHeader)) return false;

    const uint8_t* ptr = (const uint8_t*)root_val;
    const NTFS_IndexHeader* idx_hdr = (const NTFS_IndexHeader*)(ptr + sizeof(NTFS_IndexRootHeader));

    uint32_t entry_offset = sizeof(NTFS_IndexRootHeader) + idx_hdr->entries_offset;
    uint32_t depth = 0;
    uint32_t max_depth = 32;

    while (entry_offset + sizeof(NTFS_IndexEntry) <= root_len && depth++ < max_depth) {
        const NTFS_IndexEntry* entry = (const NTFS_IndexEntry*)(ptr + entry_offset);
        if (entry->length == 0 || entry_offset + entry->length > root_len) break;

        if (!(entry->flags & NTFS_INDEX_ENTRY_LAST) && entry->key_length >= sizeof(NTFS_FileNameAttr)) {
            const NTFS_FileNameAttr* fname = (const NTFS_FileNameAttr*)((const uint8_t*)entry + sizeof(NTFS_IndexEntry));
            int cmp = ntfs_filename_cmp(name, fname->filename, fname->filename_len);
            if (cmp == 0) {
                *out_file_ref = entry->file_reference;
                return true;
            }
        }

        if (entry->flags & NTFS_INDEX_ENTRY_LAST) break;
        entry_offset += entry->length;
    }

    // 5B: Traversal into $INDEX_ALLOCATION when present
    NTFS_Attribute alloc_attr;
    if (ntfs_attr_find(dir_rec, NTFS_ATTR_INDEX_ALLOCATION, "$I30", &alloc_attr) ||
        ntfs_attr_find(dir_rec, NTFS_ATTR_INDEX_ALLOCATION, NULL, &alloc_attr)) {

        uint32_t buf_size = vol->index_buffer_size;
        if (buf_size < 512) buf_size = 4096;

        uint8_t* indx_buf = (uint8_t*)kmalloc(buf_size);
        if (!indx_buf) return false;

        NTFS_File alloc_file;
        alloc_file.vol = vol; alloc_file.has_data = true; alloc_file.non_resident = alloc_attr.non_resident;
        alloc_file.is_compressed = false; alloc_file.is_encrypted = false;
        alloc_file.data_size = alloc_attr.data_size; alloc_file.initialized_size = alloc_attr.initialized_size;
        alloc_file.allocated_size = alloc_attr.allocated_size; alloc_file.resident_data = NULL; alloc_file.record = NULL;

        const uint8_t* runlist = alloc_attr.raw_attr_ptr + alloc_attr.mapping_pairs_offset;
        uint32_t runlist_len = alloc_attr.length - alloc_attr.mapping_pairs_offset;
        ntfs_decode_data_runs(runlist, runlist_len, alloc_attr.starting_vcn, &alloc_file.extent_map, NULL);

        int64_t nread = ntfs_file_read(&alloc_file, 0, indx_buf, buf_size);
        bool found = false;
        if (nread == buf_size && ntfs_indx_validate_and_fixup(indx_buf, buf_size, vol->bytes_per_sector)) {
            const NTFS_IndexBlockHeader* indx_hdr = (const NTFS_IndexBlockHeader*)indx_buf;
            uint32_t e_off = 0x18 + indx_hdr->index_hdr.entries_offset;
            while (e_off + sizeof(NTFS_IndexEntry) <= buf_size) {
                const NTFS_IndexEntry* entry = (const NTFS_IndexEntry*)(indx_buf + e_off);
                if (entry->length == 0 || e_off + entry->length > buf_size) break;

                if (!(entry->flags & NTFS_INDEX_ENTRY_LAST) && entry->key_length >= sizeof(NTFS_FileNameAttr)) {
                    const NTFS_FileNameAttr* fname = (const NTFS_FileNameAttr*)((const uint8_t*)entry + sizeof(NTFS_IndexEntry));
                    int cmp = ntfs_filename_cmp(name, fname->filename, fname->filename_len);
                    if (cmp == 0) {
                        *out_file_ref = entry->file_reference;
                        found = true;
                        break;
                    }
                }
                if (entry->flags & NTFS_INDEX_ENTRY_LAST) break;
                e_off += entry->length;
            }
        }

        kfree(indx_buf);
        ntfs_extent_map_free(&alloc_file.extent_map);
        return found;
    }

    return false;
}

// 5E: Directory Enumeration
bool ntfs_dir_enum(NTFS_VOLUME* vol, const NTFS_FileRecord* dir_rec, NTFS_DirEntry** out_entries, uint32_t* out_count) {
    if (!vol || !dir_rec || !out_entries || !out_count) return false;

    *out_entries = NULL; *out_count = 0;

    NTFS_Attribute root_attr;
    if (!ntfs_attr_find(dir_rec, NTFS_ATTR_INDEX_ROOT, "$I30", &root_attr) &&
        !ntfs_attr_find(dir_rec, NTFS_ATTR_INDEX_ROOT, NULL, &root_attr)) {
        return false;
    }

    const void* root_val = NULL; uint32_t root_len = 0;
    if (!ntfs_attr_get_resident_value(dir_rec, &root_attr, &root_val, &root_len)) return false;

    uint32_t capacity = 16;
    uint32_t count = 0;
    NTFS_DirEntry* entries = (NTFS_DirEntry*)kmalloc(capacity * sizeof(NTFS_DirEntry));
    if (!entries) return false;

    const uint8_t* ptr = (const uint8_t*)root_val;
    const NTFS_IndexHeader* idx_hdr = (const NTFS_IndexHeader*)(ptr + sizeof(NTFS_IndexRootHeader));
    uint32_t entry_offset = sizeof(NTFS_IndexRootHeader) + idx_hdr->entries_offset;

    while (entry_offset + sizeof(NTFS_IndexEntry) <= root_len) {
        const NTFS_IndexEntry* entry = (const NTFS_IndexEntry*)(ptr + entry_offset);
        if (entry->length == 0 || entry_offset + entry->length > root_len) break;

        if (!(entry->flags & NTFS_INDEX_ENTRY_LAST) && entry->key_length >= sizeof(NTFS_FileNameAttr)) {
            const NTFS_FileNameAttr* fname = (const NTFS_FileNameAttr*)((const uint8_t*)entry + sizeof(NTFS_IndexEntry));

            if (fname->namespace != 2 || count == 0) {
                if (count >= capacity) {
                    uint32_t new_cap = capacity * 2;
                    NTFS_DirEntry* new_arr = (NTFS_DirEntry*)kmalloc(new_cap * sizeof(NTFS_DirEntry));
                    if (new_arr) {
                        for (uint32_t i = 0; i < count; i++) new_arr[i] = entries[i];
                        kfree(entries);
                        entries = new_arr;
                        capacity = new_cap;
                    }
                }

                NTFS_DirEntry* de = &entries[count];
                de->record_number = (uint32_t)(entry->file_reference & 0xFFFFFFFFFFFFULL);
                de->sequence_number = (uint16_t)(entry->file_reference >> 48);
                de->is_directory = (fname->file_flags & 0x10) != 0;
                de->file_size = fname->real_size;
                de->name_space = fname->namespace;

                uint32_t nlen = fname->filename_len < 255 ? fname->filename_len : 255;
                for (uint32_t i = 0; i < nlen; i++) de->name[i] = (char)(fname->filename[i] & 0x7F);
                de->name[nlen] = '\0';
                count++;
            }
        }
        if (entry->flags & NTFS_INDEX_ENTRY_LAST) break;
        entry_offset += entry->length;
    }

    *out_entries = entries;
    *out_count = count;
    return true;
}

// 5F: Full Path Resolver
bool ntfs_resolve_path(NTFS_VOLUME* vol, const char* path, uint32_t* out_record_num) {
    if (!vol || !vol->mounted || !path || !out_record_num) return false;

    // 7E: Check Path Lookup Cache Hit
    for (int i = 0; i < NTFS_PATH_CACHE_SIZE; i++) {
        if (vol->path_cache.entries[i].valid && strcmp(vol->path_cache.entries[i].path, path) == 0) {
            vol->path_cache.hits++;
            vol->stats.path_cache_hits++;
            vol->path_cache.entries[i].access_count++;
            *out_record_num = vol->path_cache.entries[i].record_number;
            return true;
        }
    }

    vol->path_cache.misses++;
    vol->stats.path_cache_misses++;

    const char* raw_path = path;
    while (*path == '/') path++;

    if (*path == '\0') {
        *out_record_num = NTFS_ROOT_RECORD_NUM;
        return true;
    }

    uint32_t curr_rec_num = NTFS_ROOT_RECORD_NUM;
    char token[256];
    const char* p = path;

    while (*p != '\0') {
        while (*p == '/') p++;
        if (*p == '\0') break;

        uint32_t tlen = 0;
        while (*p != '\0' && *p != '/' && tlen < 255) {
            token[tlen++] = *p++;
        }
        token[tlen] = '\0';

        NTFS_FileRecord* dir_rec = ntfs_mft_read_record(vol, curr_rec_num);
        if (!dir_rec) return false;

        if (!(dir_rec->flags & NTFS_FILE_DIRECTORY)) {
            ntfs_mft_free_record(dir_rec);
            return false;
        }

        uint64_t child_ref = 0;
        bool match = ntfs_dir_lookup_entry(vol, dir_rec, token, &child_ref);
        ntfs_mft_free_record(dir_rec);

        if (!match) return false;

        curr_rec_num = (uint32_t)(child_ref & 0xFFFFFFFFFFFFULL);
    }

    *out_record_num = curr_rec_num;

    // 7E: Cache Resolved Path Lookup
    int target_idx = -1;
    uint32_t min_access = 0xFFFFFFFF;
    for (int i = 0; i < NTFS_PATH_CACHE_SIZE; i++) {
        if (!vol->path_cache.entries[i].valid) {
            target_idx = i;
            break;
        }
        if (vol->path_cache.entries[i].access_count < min_access) {
            min_access = vol->path_cache.entries[i].access_count;
            target_idx = i;
        }
    }
    if (target_idx >= 0) {
        if (vol->path_cache.entries[target_idx].valid) {
            vol->path_cache.evictions++;
            vol->stats.path_cache_evictions++;
        }
        uint32_t plen = 0;
        while (raw_path[plen] != '\0' && plen < 255) {
            vol->path_cache.entries[target_idx].path[plen] = raw_path[plen];
            plen++;
        }
        vol->path_cache.entries[target_idx].path[plen] = '\0';
        vol->path_cache.entries[target_idx].record_number = curr_rec_num;
        vol->path_cache.entries[target_idx].valid = true;
        vol->path_cache.entries[target_idx].access_count = 1;
    }

    return true;
}

// 5F: Phase 4 Integration - Open File by Path
NTFS_File* ntfs_open_file_by_path(NTFS_VOLUME* vol, const char* path) {
    uint32_t rec_num = 0;
    if (!ntfs_resolve_path(vol, path, &rec_num)) return NULL;
    return ntfs_file_open_by_record(vol, rec_num);
}

// ===========================================================================
// PHASE 6 — PRODUCTION VFS ADAPTER & CALLBACKS
// ===========================================================================

static void ntfs_vfs_strip_mount_prefix(const char* full_path, const char* mount_path, char* out_rel_path) {
    if (!full_path || !out_rel_path) return;
    if (!mount_path || strcmp(mount_path, "/") == 0) {
        strcpy(out_rel_path, full_path);
        return;
    }

    size_t mlen = strlen(mount_path);
    if (strncmp(full_path, mount_path, mlen) == 0) {
        const char* sub = full_path + mlen;
        if (sub[0] == '\0') {
            strcpy(out_rel_path, "/");
        } else {
            strcpy(out_rel_path, sub);
        }
    } else {
        strcpy(out_rel_path, full_path);
    }
}

static NTFS_VOLUME* ntfs_get_vol_from_node(VFS_Node* node) {
    if (!node) return NULL;
    if (node->parent && node->parent->private_data) {
        return (NTFS_VOLUME*)node->parent->private_data;
    }
    return (NTFS_VOLUME*)node->private_data;
}

static int ntfs_vfs_open(VFS_Node* node, const char* path) {
    if (!node || !path) return -1;
    NTFS_VOLUME* vol = ntfs_get_vol_from_node(node);
    if (!vol || !vol->mounted) return -1;

    char rel_path[256];
    VFS_Mount* mount = vfs_get_mount(path);
    const char* mpath = mount ? mount->mount_path : "/";
    ntfs_vfs_strip_mount_prefix(path, mpath, rel_path);

    NTFS_File* file = ntfs_open_file_by_path(vol, rel_path);
    if (!file) return -1;

    node->private_data = file;
    node->size = file->data_size;
    node->type = file->is_directory ? VFS_DIRECTORY : VFS_FILE;
    return 0;
}

static int ntfs_vfs_read(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    if (!node || !node->private_data || !buffer) return -1;
    NTFS_File* file = (NTFS_File*)node->private_data;
    if (file->is_directory) return -1;

    int64_t bytes_read = ntfs_file_read(file, offset, buffer, size);
    return (int)bytes_read;
}

static int ntfs_vfs_write(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return -1; // Read-only filesystem in Phase 6
}

static int ntfs_vfs_close(VFS_Node* node) {
    if (!node || !node->private_data) return -1;
    NTFS_File* file = (NTFS_File*)node->private_data;
    ntfs_file_close(file);
    node->private_data = NULL;
    return 0;
}

static int ntfs_vfs_readdir(VFS_Node* node, const char* path, int index, vfs_dirent_t* out_entry) {
    if (!node || !path || !out_entry) return -1;
    NTFS_VOLUME* vol = (node->parent && node->parent->private_data) ? (NTFS_VOLUME*)node->parent->private_data : (NTFS_VOLUME*)node->private_data;
    if (!vol || !vol->mounted) return -1;

    char rel_path[256];
    VFS_Mount* mount = vfs_get_mount(path);
    const char* mpath = mount ? mount->mount_path : "/";
    ntfs_vfs_strip_mount_prefix(path, mpath, rel_path);

    uint32_t dir_rec_num = 0;
    if (!ntfs_resolve_path(vol, rel_path, &dir_rec_num)) return -1;

    NTFS_FileRecord* dir_rec = ntfs_mft_read_record(vol, dir_rec_num);
    if (!dir_rec) return -1;
    if (!(dir_rec->flags & NTFS_FILE_DIRECTORY)) {
        ntfs_mft_free_record(dir_rec);
        return -1;
    }

    NTFS_DirEntry* entries = NULL;
    uint32_t count = 0;
    bool ok = ntfs_dir_enum(vol, dir_rec, &entries, &count);
    ntfs_mft_free_record(dir_rec);

    if (!ok || !entries || index < 0 || index >= (int)count) {
        if (entries) kfree(entries);
        return -1;
    }

    strcpy(out_entry->name, entries[index].name);
    out_entry->size = entries[index].file_size;
    out_entry->is_directory = entries[index].is_directory ? 1 : 0;
    out_entry->cluster = entries[index].record_number;

    kfree(entries);
    return 0;
}

static int ntfs_vfs_mkdir(VFS_Node* node, const char* name) {
    (void)node; (void)name; return -1;
}

static int ntfs_vfs_create(VFS_Node* node, const char* name) {
    (void)node; (void)name; return -1;
}

static int ntfs_vfs_rename(VFS_Node* node, const char* old_path, const char* new_name) {
    (void)node; (void)old_path; (void)new_name; return -1;
}

static int ntfs_vfs_delete(VFS_Node* node, const char* path) {
    (void)node; (void)path; return -1;
}

FilesystemDriver ntfs_fs_driver = {
    .name = "ntfs",
    .mount = ntfs_mount_cb,
    .open = ntfs_vfs_open,
    .read = ntfs_vfs_read,
    .write = ntfs_vfs_write,
    .close = ntfs_vfs_close,
    .readdir = ntfs_vfs_readdir,
    .mkdir = ntfs_vfs_mkdir,
    .create = ntfs_vfs_create,
    .rename = ntfs_vfs_rename,
    .delete = ntfs_vfs_delete
};

// ---------------------------------------------------------------------------
// Filesystem Registration API
// ---------------------------------------------------------------------------
void ntfs_init(void) {
    vfs_register_fs(&ntfs_fs_driver);
    display_print("[NTFS] Production VFS Driver Registered with VFS.\n");
}
