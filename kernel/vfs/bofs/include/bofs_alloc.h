#ifndef BOFS_ALLOC_H
#define BOFS_ALLOC_H

#include "bofs_format.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Return & Error Codes
 * -------------------------------------------------------------------------- */
#define BOFS_ALLOC_OK                   0
#define BOFS_ERR_METADATA_PROTECTED    -1   /* Attempt to touch reserved metadata */
#define BOFS_ERR_RESERVATION_NOT_FOUND -2   /* Invalid reservation handle */
#define BOFS_ERR_IO                    -5   /* Physical BlockDevice I/O error */
#define BOFS_ERR_RESERVATION_FULL      -12  /* Reservation table capacity exhausted */
#define BOFS_ERR_DOUBLE_FREE           -16  /* Block was already unallocated (0) */
#define BOFS_ERR_DOUBLE_ALLOC          -17  /* Block was already allocated (1) */
#define BOFS_ERR_OUT_OF_BOUNDS         -8   /* Block outside partition or data pool */
#define BOFS_ERR_INVALID_PARAM         -23  /* NULL pointer or zero count */
#define BOFS_ERR_OUT_OF_SPACE          -28  /* Insufficient free blocks (ENOSPC) */
#define BOFS_ERR_CORRUPT_BITMAP        -117 /* Malformed bitmap metadata detected */

#define BOFS_MAX_RESERVATIONS          32
#define BOFS_MAX_ALLOC_EXTENTS         16

/* Fragmented Allocation Extent List */
typedef struct {
    uint64_t start_block;
    uint32_t count;
} bofs_alloc_extent_t;

typedef struct {
    uint32_t count;               /* Number of valid extents in list */
    uint32_t total_blocks;        /* Sum of blocks across all extents */
    bofs_alloc_extent_t extents[BOFS_MAX_ALLOC_EXTENTS];
} bofs_alloc_extent_list_t;

/* In-Memory Reservation Handle */
typedef struct {
    uint32_t id;
    bool     active;
    uint64_t start_block;
    uint32_t count;
} bofs_reservation_t;

/* --------------------------------------------------------------------------
 * BOFS Block Allocator Context
 * -------------------------------------------------------------------------- */
typedef struct {
    BlockDevice* dev;
    bofs_superblock_t sb;
    uint64_t data_pool_start;     /* First allocatable block */
    uint64_t data_pool_end;       /* Last allocatable block (total_blocks - 1) */
    uint64_t free_blocks_count;   /* Cached count of unallocated blocks */
    uint64_t last_alloc_cursor;   /* Sequential allocation search cursor */

    /* Active Bitmap Block Cache */
    uint8_t  cached_bitmap[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
    uint64_t cached_bitmap_block; /* Absolute partition block number currently in cache */
    bool     cached_bitmap_dirty; /* True if buffer contains uncommitted bit changes */

    /* Reservation Table */
    bofs_reservation_t reservations[BOFS_MAX_RESERVATIONS];
    uint32_t next_reservation_id;
} bofs_allocator_t;

/* --------------------------------------------------------------------------
 * Core Allocator Public API
 * -------------------------------------------------------------------------- */

/* Initialize allocator context from physical BlockDevice */
int bofs_allocator_init(bofs_allocator_t* alloc, BlockDevice* dev);

/* Flush any dirty cached bitmap blocks and commit to storage */
int bofs_allocator_flush(bofs_allocator_t* alloc);

/* Allocate a single 4KB block from data pool */
int bofs_alloc_block(bofs_allocator_t* alloc, uint64_t* out_block);

/* Allocate N contiguous blocks */
int bofs_alloc_blocks_contiguous(bofs_allocator_t* alloc, uint32_t count, uint64_t* out_start_block);

/* Allocate N blocks across fragmented extents */
int bofs_alloc_blocks_fragmented(bofs_allocator_t* alloc, uint32_t count, bofs_alloc_extent_list_t* out_list);

/* Reserve N contiguous blocks without immediately marking persistent bitmap */
int bofs_alloc_reserve_blocks(bofs_allocator_t* alloc, uint32_t count, uint32_t* out_res_id, uint64_t* out_start_block);

/* Commit an active reservation (marks bitmap 1 and flushes) */
int bofs_alloc_commit_reservation(bofs_allocator_t* alloc, uint32_t res_id);

/* Rollback an active reservation (releases in-memory lock without disk writes) */
int bofs_alloc_rollback_reservation(bofs_allocator_t* alloc, uint32_t res_id);

/* Free a previously allocated single block */
int bofs_free_block(bofs_allocator_t* alloc, uint64_t block_num);

/* Free N previously allocated contiguous blocks */
int bofs_free_blocks(bofs_allocator_t* alloc, uint64_t start_block, uint32_t count);

/* Count actual free blocks by scanning bitmap */
uint64_t bofs_count_free_blocks(bofs_allocator_t* alloc);

/* Validate persistent bitmap consistency and geometry */
int bofs_validate_bitmap(bofs_allocator_t* alloc);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_ALLOC_H */
