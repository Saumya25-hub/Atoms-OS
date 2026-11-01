/*
 * ATOMS OS — USB Runtime Storage & NTFS File-Backed VirtIO-Block Subsystem
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 2 & 3: Direct USB FreeBSD Runtime Integration
 */

#ifndef ATOMS_RUNTIME_STORAGE_H
#define ATOMS_RUNTIME_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/hypervisor/include/virtio_blk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RUNTIME_STORAGE_IMAGE_NAME       "FreeBSD_Runtime.img"
#define RUNTIME_STORAGE_EXPECTED_SECTORS 41943040ULL /* 20 GiB (41,943,040 sectors of 512B) */

/* Runtime Storage Forensic Telemetry */
typedef struct {
    bool     usb_detected;
    bool     usb_msc_ready;
    bool     ntfs_mounted;
    bool     image_located;
    char     volume_label[32];
    char     image_path[64];
    uint64_t image_size_bytes;
    uint64_t total_sectors;
    uint32_t bytes_per_sector;
    uint32_t cluster_size;

    /* I/O Performance & Health Counters */
    uint64_t total_read_requests;
    uint64_t total_sectors_read;
    uint64_t total_bytes_read;
    uint64_t failed_read_requests;
    uint64_t last_failed_sector;
    uint32_t last_latency_us;
    uint32_t max_latency_us;
    uint32_t avg_latency_us;
    uint64_t total_latency_us;

    /* Status string for dashboard */
    char     status_msg[96];

    /* Phase 4 Computed Telemetry & Forensics */
    char     root_source[48];
    char     fail_step[32];
    uint32_t fail_reason_code;
    uint32_t fragment_count;
    bool     is_sparse;
    bool     is_compressed;
    char     raw_runlist_hex[64];
} RuntimeStorageTelemetry;

extern RuntimeStorageTelemetry g_runtime_storage_telemetry;

/* Core APIs */
bool runtime_storage_init(void);
bool runtime_storage_is_available(void);
VirtIOBlock *runtime_storage_create_guest_blk(void);
const RuntimeStorageTelemetry *runtime_storage_get_telemetry(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_RUNTIME_STORAGE_H */

