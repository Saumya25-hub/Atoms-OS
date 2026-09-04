#ifndef BOFS_FILE_H
#define BOFS_FILE_H

#include "bofs_format.h"
#include "bofs_alloc.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Return & Error Codes for File & Metadata Engine
 * -------------------------------------------------------------------------- */
#define BOFS_FILE_OK                     0
#define BOFS_ERR_METADATA_PROTECTED     -1   /* Attempt to free/modify reserved metadata */
#define BOFS_ERR_NOT_FOUND              -2   /* Inode or extent not found */
#define BOFS_ERR_IO                     -5   /* Physical BlockDevice I/O failure */
#define BOFS_ERR_OUT_OF_BOUNDS          -8   /* Inode or block number beyond volume capacity */
#define BOFS_ERR_INODE_FULL             -11  /* Inode bitmap exhausted (no free inodes) */
#define BOFS_ERR_DOUBLE_FREE            -16  /* Inode was already unallocated (bit = 0) */
#define BOFS_ERR_DOUBLE_ALLOC           -17  /* Inode was already allocated (bit = 1) */
#define BOFS_ERR_INVALID_PARAM          -23  /* NULL pointer or invalid parameters */
#define BOFS_ERR_OUT_OF_SPACE           -28  /* No free data blocks available (ENOSPC) */
#define BOFS_ERR_CHECKSUM_MISMATCH      -4   /* CRC32 integrity failure (matches BOFS_VALIDATOR) */
#define BOFS_ERR_OVERFLOW               -75  /* Integer or offset arithmetic overflow */
#define BOFS_ERR_CORRUPT_METADATA       -117 /* Malformed metadata record */
#define BOFS_ERR_STALE_HANDLE           -118 /* Inode generation mismatch / recycled object */
#define BOFS_ERR_EXTENT_LIMIT           -119 /* Direct + indirect extent capacity exceeded */

#define BOFS_EXTENTS_PER_INDIRECT_BLOCK 170U /* 170 * 24 bytes = 4,080 bytes */

/* --------------------------------------------------------------------------
 * Structure: Indirect Extent Block (4,096 Bytes)
 * -------------------------------------------------------------------------- */
typedef struct {
    bofs_extent_t extents[BOFS_EXTENTS_PER_INDIRECT_BLOCK]; /* Offset 0x000 - 0xFEF (4,080 B) */
    uint8_t       reserved[12];                             /* Offset 0xFF0 - 0xFFB (12 B) */
    uint32_t      checksum;                                 /* Offset 0xFFC: CRC32 of first 4,092 B */
} __attribute__((packed)) bofs_indirect_block_t;

_Static_assert(sizeof(bofs_indirect_block_t) == 4096, "BOFS: bofs_indirect_block_t must be 4096 bytes");

/* --------------------------------------------------------------------------
 * BOFS In-Memory Filesystem Context
 * -------------------------------------------------------------------------- */
typedef struct {
    BlockDevice*      dev;
    bofs_allocator_t* alloc;
    bofs_superblock_t sb;

    /* Inode Bitmap Block Cache */
    uint8_t           cached_inode_bmp[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
    uint64_t          cached_inode_bmp_block;
    bool              cached_inode_bmp_dirty;
} bofs_file_system_t;

/* --------------------------------------------------------------------------
 * BOFS In-Memory File Handle
 * -------------------------------------------------------------------------- */
typedef struct {
    bofs_file_system_t* fs;
    uint64_t            inode_num;
    uint32_t            generation;
    bofs_inode_t        inode;
    bool                is_open;
    bool                dirty;
} bofs_file_t;

/* --------------------------------------------------------------------------
 * Public API: Filesystem & Inode Management
 * -------------------------------------------------------------------------- */

/* Initialize filesystem context with underlying BlockDevice and Allocator */
int bofs_fs_init(bofs_file_system_t* fs, BlockDevice* dev, bofs_allocator_t* alloc);

/* Flush any dirty inode bitmap or cached metadata blocks to storage */
int bofs_fs_flush(bofs_file_system_t* fs);

/* Count free inodes by authoritative scan of inode bitmap */
uint64_t bofs_count_free_inodes(bofs_file_system_t* fs);

/* Allocate a free Inode from the Inode bitmap */
int bofs_inode_alloc(bofs_file_system_t* fs, uint64_t* out_inode_num);

/* Release an allocated Inode back to the Inode bitmap */
int bofs_inode_free(bofs_file_system_t* fs, uint64_t inode_num);

/* Read an Inode from disk and validate CRC32 checksum */
int bofs_inode_read(bofs_file_system_t* fs, uint64_t inode_num, bofs_inode_t* out_inode);

/* Write an Inode to disk, updating CRC32 checksum and flushing */
int bofs_inode_write(bofs_file_system_t* fs, const bofs_inode_t* inode);

/* --------------------------------------------------------------------------
 * Public API: File Lifecycle & Data Operations
 * -------------------------------------------------------------------------- */

/* Create a new regular file object, allocating an Inode and initializing metadata */
int bofs_file_create(bofs_file_system_t* fs, uint16_t mode, bofs_file_t* out_file);

/* Open an existing file object by Inode number, validating generation and checksum */
int bofs_file_open(bofs_file_system_t* fs, uint64_t inode_num, uint32_t expected_generation, bofs_file_t* out_file);

/* Close an open file object, flushing metadata if dirty */
int bofs_file_close(bofs_file_t* file);

/* Read data from file at specified logical byte offset */
int bofs_file_read(bofs_file_t* file, uint64_t offset, void* buffer, uint64_t length, uint64_t* out_read);

/* Write data to file at specified logical byte offset (supports overwrite, append, growth) */
int bofs_file_write(bofs_file_t* file, uint64_t offset, const void* buffer, uint64_t length, uint64_t* out_written);

/* Record an explicit sparse hole (no physical blocks allocated) */
int bofs_file_write_sparse(bofs_file_t* file, uint64_t offset, uint64_t length);

/* Truncate or shrink file to new size (releases unused blocks via Phase 4 allocator) */
int bofs_file_truncate(bofs_file_t* file, uint64_t new_size);

/* Low-level file object deletion: releases all data blocks and marks Inode free */
int bofs_file_delete(bofs_file_system_t* fs, uint64_t inode_num, uint32_t generation);

/* Logical block to physical block translation (bmap) */
int bofs_file_bmap(bofs_file_t* file, uint64_t logical_block, uint64_t* out_phys_block, bool* out_is_sparse);

#ifdef __cplusplus
}
#endif

#endif /* BOFS_FILE_H */
