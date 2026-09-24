/*
 * ATOMS OS — VirtIO Block Device Header
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_BLK_H
#define ATOMS_VIRTIO_BLK_H

#include "virtio_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIRTIO_BLK_DEFAULT_SECTORS          8388608ULL /* 4 GB RAM Disk (8,388,608 sectors) */
#define VIRTIO_BLK_CHUNK_SIZE               (2 * 1024 * 1024ULL) /* 2 MB per chunk */
#define VIRTIO_BLK_SECTORS_PER_CHUNK        (VIRTIO_BLK_CHUNK_SIZE / 512ULL) /* 4096 sectors */
#define VIRTIO_BLK_SERIAL                   "ATOMS-VBLK-001"

/* VirtIO Block Device Instance */
typedef struct virtio_blk_dev {
    VirtIODevice *base;

    /* Isolated RAM-Backed Storage */
    uint64_t total_sectors;
    uint8_t *storage_backing; /* Flat buffer pointer (if allocated contiguous or preloaded) */
    uint8_t **chunk_table;    /* Dynamic chunk array for >=4GB sparse RAM disk backing */
    uint32_t chunk_count;
    bool read_only;

    /* Metrics & Statistics */
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t total_flushes;
    uint64_t total_bytes_read;
    uint64_t total_bytes_written;
} VirtIOBlock;

/* Core APIs */
VirtIOBlock *virtio_blk_create(uint64_t sector_count, bool read_only);
void virtio_blk_destroy(VirtIOBlock *blk);
bool virtio_blk_read_sectors(VirtIOBlock *blk, uint64_t start_sector, uint32_t num_sectors, uint8_t *dest);
bool virtio_blk_write_sectors(VirtIOBlock *blk, uint64_t start_sector, uint32_t num_sectors, const uint8_t *src);
bool virtio_blk_self_test(VirtIOBlock *blk);

/* Request Processing */
void virtio_blk_process_queue(VirtIOBlock *blk);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_BLK_H */
