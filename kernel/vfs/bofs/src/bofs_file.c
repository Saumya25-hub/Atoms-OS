#include "kernel/vfs/bofs/include/bofs_file.h"
#include "kernel/vfs/bofs/include/bofs_validator.h"
#include "kernel/core/lib/include/string.h"

/* --------------------------------------------------------------------------
 * Internal Helper: Block to Sector Translation
 * -------------------------------------------------------------------------- */
static inline bool bofs_fs_block_to_lba(const bofs_file_system_t* fs, uint64_t block_idx, uint64_t* out_lba, uint32_t* out_sec_count) {
    if (!fs || !fs->dev || fs->sb.sector_size == 0) return false;
    uint32_t spb = BOFS_BLOCK_SIZE / (uint32_t)fs->sb.sector_size;
    if (spb == 0) return false;
    if (block_idx > (UINT64_MAX / spb)) return false;

    *out_lba = block_idx * spb;
    *out_sec_count = spb;
    return true;
}

/* --------------------------------------------------------------------------
 * Internal Helper: Inode Bitmap Block Cache Manager
 * -------------------------------------------------------------------------- */
static int bofs_load_inode_bitmap_block(bofs_file_system_t* fs, uint64_t bmp_disk_block) {
    if (!fs || !fs->dev) return BOFS_ERR_INVALID_PARAM;
    if (fs->cached_inode_bmp_block == bmp_disk_block) return BOFS_FILE_OK;

    /* Flush dirty cached block before eviction */
    if (fs->cached_inode_bmp_dirty && fs->cached_inode_bmp_block != (uint64_t)-1) {
        int flush_res = bofs_fs_flush(fs);
        if (flush_res != BOFS_FILE_OK) return flush_res;
    }

    uint64_t lba;
    uint32_t count;
    if (!bofs_fs_block_to_lba(fs, bmp_disk_block, &lba, &count)) return BOFS_ERR_OUT_OF_BOUNDS;

    if (!fs->dev->read(fs->dev, lba, count, fs->cached_inode_bmp)) {
        return BOFS_ERR_IO;
    }

    fs->cached_inode_bmp_block = bmp_disk_block;
    fs->cached_inode_bmp_dirty = false;
    return BOFS_FILE_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_fs_init & bofs_fs_flush
 * -------------------------------------------------------------------------- */
int bofs_fs_init(bofs_file_system_t* fs, BlockDevice* dev, bofs_allocator_t* alloc) {
    if (!fs || !dev || !alloc) return BOFS_ERR_INVALID_PARAM;
    memset(fs, 0, sizeof(bofs_file_system_t));
    fs->dev = dev;
    fs->alloc = alloc;

    /* Read Superblock at Block 0 */
    uint32_t spb = BOFS_BLOCK_SIZE / (uint32_t)dev->sector_size;
    if (!dev->read(dev, 0, spb, &fs->sb)) {
        return BOFS_ERR_IO;
    }

    uint64_t total_dev_blocks = dev->sector_count / spb;
    int sb_res = bofs_validate_superblock(&fs->sb, total_dev_blocks);
    if (sb_res != BOFS_VALID_OK) {
        return BOFS_ERR_CORRUPT_METADATA;
    }

    fs->cached_inode_bmp_block = (uint64_t)-1;
    fs->cached_inode_bmp_dirty = false;
    return BOFS_FILE_OK;
}

int bofs_fs_flush(bofs_file_system_t* fs) {
    if (!fs || !fs->dev) return BOFS_ERR_INVALID_PARAM;

    if (fs->cached_inode_bmp_dirty && fs->cached_inode_bmp_block != (uint64_t)-1) {
        uint64_t lba;
        uint32_t count;
        if (!bofs_fs_block_to_lba(fs, fs->cached_inode_bmp_block, &lba, &count)) {
            return BOFS_ERR_OUT_OF_BOUNDS;
        }

        if (!fs->dev->write(fs->dev, lba, count, fs->cached_inode_bmp)) {
            return BOFS_ERR_IO;
        }
        fs->cached_inode_bmp_dirty = false;
    }

    if (fs->alloc) {
        bofs_allocator_flush(fs->alloc);
    }

    if (fs->dev->flush) {
        if (!fs->dev->flush(fs->dev)) {
            return BOFS_ERR_IO;
        }
    }

    return BOFS_FILE_OK;
}

/* --------------------------------------------------------------------------
 * Public API: Inode Allocation & Deallocation Lifecycle
 * -------------------------------------------------------------------------- */
int bofs_inode_alloc(bofs_file_system_t* fs, uint64_t* out_inode_num) {
    if (!fs || !out_inode_num) return BOFS_ERR_INVALID_PARAM;

    /* Scan from Inode 16 (first user Inode) to total_inodes - 1 */
    for (uint64_t ino = BOFS_FIRST_USER_INODE; ino < fs->sb.total_inodes; ino++) {
        uint64_t bmp_blk_offset = ino / 32768ULL;
        uint64_t bmp_disk_block = fs->sb.inode_bitmap_start_block + bmp_blk_offset;
        uint32_t local_bit = (uint32_t)(ino % 32768ULL);

        int load_res = bofs_load_inode_bitmap_block(fs, bmp_disk_block);
        if (load_res != BOFS_FILE_OK) return load_res;

        uint8_t mask = (uint8_t)(1U << (local_bit & 7));
        uint32_t byte_idx = local_bit >> 3;

        if (!(fs->cached_inode_bmp[byte_idx] & mask)) {
            /* Free Inode located: claim bit */
            fs->cached_inode_bmp[byte_idx] |= mask;
            fs->cached_inode_bmp_dirty = true;

            int flush_res = bofs_fs_flush(fs);
            if (flush_res != BOFS_FILE_OK) {
                fs->cached_inode_bmp[byte_idx] &= (uint8_t)~mask;
                fs->cached_inode_bmp_dirty = false;
                return flush_res;
            }

            if (fs->sb.free_inodes > 0) {
                fs->sb.free_inodes--;
            }

            *out_inode_num = ino;
            return BOFS_FILE_OK;
        }
    }

    return BOFS_ERR_INODE_FULL;
}

int bofs_inode_free(bofs_file_system_t* fs, uint64_t inode_num) {
    if (!fs) return BOFS_ERR_INVALID_PARAM;
    if (inode_num < BOFS_FIRST_USER_INODE) return BOFS_ERR_METADATA_PROTECTED;
    if (inode_num >= fs->sb.total_inodes) return BOFS_ERR_OUT_OF_BOUNDS;

    uint64_t bmp_blk_offset = inode_num / 32768ULL;
    uint64_t bmp_disk_block = fs->sb.inode_bitmap_start_block + bmp_blk_offset;
    uint32_t local_bit = (uint32_t)(inode_num % 32768ULL);

    int load_res = bofs_load_inode_bitmap_block(fs, bmp_disk_block);
    if (load_res != BOFS_FILE_OK) return load_res;

    uint8_t mask = (uint8_t)(1U << (local_bit & 7));
    uint32_t byte_idx = local_bit >> 3;

    /* Double-free check */
    if (!(fs->cached_inode_bmp[byte_idx] & mask)) {
        return BOFS_ERR_DOUBLE_FREE;
    }

    fs->cached_inode_bmp[byte_idx] &= (uint8_t)~mask;
    fs->cached_inode_bmp_dirty = true;

    int flush_res = bofs_fs_flush(fs);
    if (flush_res != BOFS_FILE_OK) {
        fs->cached_inode_bmp[byte_idx] |= mask;
        fs->cached_inode_bmp_dirty = false;
        return flush_res;
    }

    fs->sb.free_inodes++;
    return BOFS_FILE_OK;
}

uint64_t bofs_count_free_inodes(bofs_file_system_t* fs) {
    if (!fs || !fs->dev) return 0;
    uint64_t free_count = 0;

    for (uint64_t ino = BOFS_FIRST_USER_INODE; ino < fs->sb.total_inodes; ino++) {
        uint64_t bmp_blk_offset = ino / 32768ULL;
        uint64_t bmp_disk_block = fs->sb.inode_bitmap_start_block + bmp_blk_offset;
        uint32_t local_bit = (uint32_t)(ino % 32768ULL);

        if (bofs_load_inode_bitmap_block(fs, bmp_disk_block) != BOFS_FILE_OK) return 0;

        uint8_t mask = (uint8_t)(1U << (local_bit & 7));
        uint32_t byte_idx = local_bit >> 3;

        if (!(fs->cached_inode_bmp[byte_idx] & mask)) {
            free_count++;
        }
    }
    return free_count;
}

/* --------------------------------------------------------------------------
 * Public API: Inode Read & Write (Persistence + Checksum)
 * -------------------------------------------------------------------------- */
int bofs_inode_read(bofs_file_system_t* fs, uint64_t inode_num, bofs_inode_t* out_inode) {
    if (!fs || !fs->dev || !out_inode) return BOFS_ERR_INVALID_PARAM;
    if (inode_num >= fs->sb.total_inodes) return BOFS_ERR_OUT_OF_BOUNDS;

    uint64_t block_offset = inode_num / BOFS_INODES_PER_BLOCK;
    uint64_t disk_block = fs->sb.inode_table_start_block + block_offset;
    uint32_t slot_index = (uint32_t)(inode_num % BOFS_INODES_PER_BLOCK);

    uint8_t block_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
    uint64_t lba;
    uint32_t sec_count;
    if (!bofs_fs_block_to_lba(fs, disk_block, &lba, &sec_count)) return BOFS_ERR_OUT_OF_BOUNDS;

    if (!fs->dev->read(fs->dev, lba, sec_count, block_buf)) {
        return BOFS_ERR_IO;
    }

    bofs_inode_t* disk_ino = (bofs_inode_t*)(block_buf + (slot_index * sizeof(bofs_inode_t)));
    memcpy(out_inode, disk_ino, sizeof(bofs_inode_t));

    /* Inode Validation */
    if (out_inode->magic != BOFS_INODE_MAGIC) {
        return BOFS_ERR_CORRUPT_METADATA;
    }
    if (out_inode->inode_num != inode_num) {
        return BOFS_ERR_CORRUPT_METADATA;
    }

    uint32_t expected_crc = bofs_crc32(out_inode, offsetof(bofs_inode_t, checksum));
    if (out_inode->checksum != expected_crc) {
        return BOFS_ERR_CHECKSUM_MISMATCH;
    }

    return BOFS_FILE_OK;
}

int bofs_inode_write(bofs_file_system_t* fs, const bofs_inode_t* inode) {
    if (!fs || !fs->dev || !inode) return BOFS_ERR_INVALID_PARAM;
    if (inode->inode_num >= fs->sb.total_inodes) return BOFS_ERR_OUT_OF_BOUNDS;

    uint64_t block_offset = inode->inode_num / BOFS_INODES_PER_BLOCK;
    uint64_t disk_block = fs->sb.inode_table_start_block + block_offset;
    uint32_t slot_index = (uint32_t)(inode->inode_num % BOFS_INODES_PER_BLOCK);

    uint8_t block_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
    uint64_t lba;
    uint32_t sec_count;
    if (!bofs_fs_block_to_lba(fs, disk_block, &lba, &sec_count)) return BOFS_ERR_OUT_OF_BOUNDS;

    /* Read existing 4KB Inode Table block */
    if (!fs->dev->read(fs->dev, lba, sec_count, block_buf)) {
        return BOFS_ERR_IO;
    }

    bofs_inode_t* target_slot = (bofs_inode_t*)(block_buf + (slot_index * sizeof(bofs_inode_t)));
    memcpy(target_slot, inode, sizeof(bofs_inode_t));

    /* Recalculate CRC32 checksum before write */
    target_slot->checksum = bofs_crc32(target_slot, offsetof(bofs_inode_t, checksum));

    if (!fs->dev->write(fs->dev, lba, sec_count, block_buf)) {
        return BOFS_ERR_IO;
    }

    if (fs->dev->flush) {
        if (!fs->dev->flush(fs->dev)) {
            return BOFS_ERR_IO;
        }
    }

    return BOFS_FILE_OK;
}

/* --------------------------------------------------------------------------
 * Public API: File Lifecycle (Create, Open, Close)
 * -------------------------------------------------------------------------- */
int bofs_file_create(bofs_file_system_t* fs, uint16_t mode, bofs_file_t* out_file) {
    if (!fs || !out_file) return BOFS_ERR_INVALID_PARAM;

    uint64_t ino_num = 0;
    int a_res = bofs_inode_alloc(fs, &ino_num);
    if (a_res != BOFS_FILE_OK) return a_res;

    /* Inspect existing slot on disk to increment generation counter */
    uint64_t block_offset = ino_num / BOFS_INODES_PER_BLOCK;
    uint64_t disk_block = fs->sb.inode_table_start_block + block_offset;
    uint32_t slot_index = (uint32_t)(ino_num % BOFS_INODES_PER_BLOCK);

    uint8_t block_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
    uint64_t lba;
    uint32_t sec_count;
    uint32_t prev_gen = 0;

    if (bofs_fs_block_to_lba(fs, disk_block, &lba, &sec_count)) {
        if (fs->dev->read(fs->dev, lba, sec_count, block_buf)) {
            bofs_inode_t* old_ino = (bofs_inode_t*)(block_buf + (slot_index * sizeof(bofs_inode_t)));
            prev_gen = old_ino->generation;
        }
    }

    bofs_inode_t new_ino;
    memset(&new_ino, 0, sizeof(bofs_inode_t));
    new_ino.magic = BOFS_INODE_MAGIC;
    new_ino.generation = (prev_gen == 0) ? 1 : (prev_gen + 1);
    new_ino.inode_num = ino_num;
    new_ino.mode = BOFS_S_IFREG | (mode & 0777U);
    new_ino.flags = 0;
    new_ino.uid = 0;
    new_ino.gid = 0;
    new_ino.link_count = 1;
    new_ino.size_bytes = 0;
    new_ino.allocated_blocks = 0;
    new_ino.indirect_block = 0;
    new_ino.double_indirect_block = 0;

    int wr_res = bofs_inode_write(fs, &new_ino);
    if (wr_res != BOFS_FILE_OK) {
        bofs_inode_free(fs, ino_num);
        return wr_res;
    }

    out_file->fs = fs;
    out_file->inode_num = ino_num;
    out_file->generation = new_ino.generation;
    out_file->inode = new_ino;
    out_file->is_open = true;
    out_file->dirty = false;

    return BOFS_FILE_OK;
}

int bofs_file_open(bofs_file_system_t* fs, uint64_t inode_num, uint32_t expected_generation, bofs_file_t* out_file) {
    if (!fs || !out_file || inode_num >= fs->sb.total_inodes) return BOFS_ERR_INVALID_PARAM;

    bofs_inode_t ino;
    int rd_res = bofs_inode_read(fs, inode_num, &ino);
    if (rd_res != BOFS_FILE_OK) return rd_res;

    /* Stale Reference / Generation Guard */
    if (expected_generation != 0 && ino.generation != expected_generation) {
        return BOFS_ERR_STALE_HANDLE;
    }

    /* Verify object is a regular file */
    if ((ino.mode & BOFS_S_IFMT) != BOFS_S_IFREG) {
        return BOFS_ERR_INVALID_PARAM;
    }

    out_file->fs = fs;
    out_file->inode_num = inode_num;
    out_file->generation = ino.generation;
    out_file->inode = ino;
    out_file->is_open = true;
    out_file->dirty = false;

    return BOFS_FILE_OK;
}

int bofs_file_close(bofs_file_t* file) {
    if (!file || !file->is_open) return BOFS_ERR_INVALID_PARAM;

    if (file->dirty) {
        int wr_res = bofs_inode_write(file->fs, &file->inode);
        if (wr_res != BOFS_FILE_OK) return wr_res;
        file->dirty = false;
    }

    file->is_open = false;
    return BOFS_FILE_OK;
}

/* --------------------------------------------------------------------------
 * Extent Mapping & BMap (Logical Block to Physical Block)
 * -------------------------------------------------------------------------- */
int bofs_file_bmap(bofs_file_t* file, uint64_t logical_block, uint64_t* out_phys_block, bool* out_is_sparse) {
    if (!file || !file->is_open || !out_phys_block || !out_is_sparse) return BOFS_ERR_INVALID_PARAM;

    *out_phys_block = 0;
    *out_is_sparse = false;

    /* 1. Search Direct Extents */
    for (uint32_t i = 0; i < BOFS_INODE_DIRECT_EXTENTS; i++) {
        bofs_extent_t* ext = &file->inode.direct_extents[i];
        if (ext->flags & BOFS_EXTENT_FLAG_VALID) {
            uint64_t start = ext->logical_block;
            uint64_t end = start + ext->block_count;
            if (logical_block >= start && logical_block < end) {
                if (ext->flags & BOFS_EXTENT_FLAG_SPARSE) {
                    *out_is_sparse = true;
                    return BOFS_FILE_OK;
                }
                *out_phys_block = ext->physical_block + (logical_block - start);
                return BOFS_FILE_OK;
            }
        }
    }

    /* 2. Search Indirect Extent Block */
    if (file->inode.indirect_block != 0) {
        uint8_t ind_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
        uint64_t lba;
        uint32_t sec_count;
        if (!bofs_fs_block_to_lba(file->fs, file->inode.indirect_block, &lba, &sec_count)) {
            return BOFS_ERR_OUT_OF_BOUNDS;
        }

        if (!file->fs->dev->read(file->fs->dev, lba, sec_count, ind_buf)) {
            return BOFS_ERR_IO;
        }

        bofs_indirect_block_t* ind = (bofs_indirect_block_t*)ind_buf;
        uint32_t exp_crc = bofs_crc32(ind, offsetof(bofs_indirect_block_t, checksum));
        if (ind->checksum != exp_crc) {
            return BOFS_ERR_CHECKSUM_MISMATCH;
        }

        for (uint32_t j = 0; j < BOFS_EXTENTS_PER_INDIRECT_BLOCK; j++) {
            bofs_extent_t* ext = &ind->extents[j];
            if (ext->flags & BOFS_EXTENT_FLAG_VALID) {
                uint64_t start = ext->logical_block;
                uint64_t end = start + ext->block_count;
                if (logical_block >= start && logical_block < end) {
                    if (ext->flags & BOFS_EXTENT_FLAG_SPARSE) {
                        *out_is_sparse = true;
                        return BOFS_FILE_OK;
                    }
                    *out_phys_block = ext->physical_block + (logical_block - start);
                    return BOFS_FILE_OK;
                }
            }
        }
    }

    return BOFS_ERR_NOT_FOUND;
}

/* --------------------------------------------------------------------------
 * Internal Helper: Append or Merge Extent
 * -------------------------------------------------------------------------- */
static int bofs_file_append_extent(bofs_file_t* file, uint64_t logical_block, uint64_t physical_block, uint32_t count, uint32_t flags) {
    if (!file || count == 0) return BOFS_ERR_INVALID_PARAM;

    /* Check if we can merge with the last valid direct extent */
    int last_direct = -1;
    for (int i = 0; i < (int)BOFS_INODE_DIRECT_EXTENTS; i++) {
        if (file->inode.direct_extents[i].flags & BOFS_EXTENT_FLAG_VALID) {
            last_direct = i;
        }
    }

    if (file->inode.indirect_block == 0 && last_direct >= 0) {
        bofs_extent_t* prev = &file->inode.direct_extents[last_direct];
        if (prev->flags == (flags | BOFS_EXTENT_FLAG_VALID)) {
            if (flags & BOFS_EXTENT_FLAG_SPARSE) {
                if (prev->logical_block + prev->block_count == logical_block) {
                    prev->block_count += count;
                    file->dirty = true;
                    return BOFS_FILE_OK;
                }
            } else {
                if (prev->logical_block + prev->block_count == logical_block &&
                    prev->physical_block + prev->block_count == physical_block) {
                    prev->block_count += count;
                    file->dirty = true;
                    return BOFS_FILE_OK;
                }
            }
        }
    }

    /* Try to insert into direct extents slot */
    for (uint32_t i = 0; i < BOFS_INODE_DIRECT_EXTENTS; i++) {
        if (!(file->inode.direct_extents[i].flags & BOFS_EXTENT_FLAG_VALID)) {
            file->inode.direct_extents[i].logical_block = logical_block;
            file->inode.direct_extents[i].physical_block = physical_block;
            file->inode.direct_extents[i].block_count = count;
            file->inode.direct_extents[i].flags = flags | BOFS_EXTENT_FLAG_VALID;
            file->dirty = true;
            return BOFS_FILE_OK;
        }
    }

    /* Direct extents are exhausted: transition to Indirect Extent Block */
    uint8_t ind_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
    uint64_t lba;
    uint32_t sec_count;

    if (file->inode.indirect_block == 0) {
        /* Allocate new block for 1st-tier indirect extents */
        uint64_t new_ind_blk = 0;
        int alloc_res = bofs_alloc_block(file->fs->alloc, &new_ind_blk);
        if (alloc_res != BOFS_ALLOC_OK) return alloc_res;

        file->inode.indirect_block = new_ind_blk;
        file->inode.allocated_blocks++;

        memset(ind_buf, 0, BOFS_BLOCK_SIZE);
        bofs_indirect_block_t* ind = (bofs_indirect_block_t*)ind_buf;
        ind->extents[0].logical_block = logical_block;
        ind->extents[0].physical_block = physical_block;
        ind->extents[0].block_count = count;
        ind->extents[0].flags = flags | BOFS_EXTENT_FLAG_VALID;
        ind->checksum = bofs_crc32(ind, offsetof(bofs_indirect_block_t, checksum));

        if (!bofs_fs_block_to_lba(file->fs, new_ind_blk, &lba, &sec_count)) {
            return BOFS_ERR_OUT_OF_BOUNDS;
        }
        if (!file->fs->dev->write(file->fs->dev, lba, sec_count, ind_buf)) {
            return BOFS_ERR_IO;
        }
        file->dirty = true;
        return BOFS_FILE_OK;
    }

    /* Load existing indirect block */
    if (!bofs_fs_block_to_lba(file->fs, file->inode.indirect_block, &lba, &sec_count)) {
        return BOFS_ERR_OUT_OF_BOUNDS;
    }
    if (!file->fs->dev->read(file->fs->dev, lba, sec_count, ind_buf)) {
        return BOFS_ERR_IO;
    }

    bofs_indirect_block_t* ind = (bofs_indirect_block_t*)ind_buf;
    uint32_t exp_crc = bofs_crc32(ind, offsetof(bofs_indirect_block_t, checksum));
    if (ind->checksum != exp_crc) {
        return BOFS_ERR_CHECKSUM_MISMATCH;
    }

    /* Check merge with last indirect extent */
    int last_ind = -1;
    for (int j = 0; j < (int)BOFS_EXTENTS_PER_INDIRECT_BLOCK; j++) {
        if (ind->extents[j].flags & BOFS_EXTENT_FLAG_VALID) {
            last_ind = j;
        }
    }

    if (last_ind >= 0) {
        bofs_extent_t* prev = &ind->extents[last_ind];
        if (prev->flags == (flags | BOFS_EXTENT_FLAG_VALID)) {
            if (flags & BOFS_EXTENT_FLAG_SPARSE) {
                if (prev->logical_block + prev->block_count == logical_block) {
                    prev->block_count += count;
                    ind->checksum = bofs_crc32(ind, offsetof(bofs_indirect_block_t, checksum));
                    file->fs->dev->write(file->fs->dev, lba, sec_count, ind_buf);
                    file->dirty = true;
                    return BOFS_FILE_OK;
                }
            } else {
                if (prev->logical_block + prev->block_count == logical_block &&
                    prev->physical_block + prev->block_count == physical_block) {
                    prev->block_count += count;
                    ind->checksum = bofs_crc32(ind, offsetof(bofs_indirect_block_t, checksum));
                    file->fs->dev->write(file->fs->dev, lba, sec_count, ind_buf);
                    file->dirty = true;
                    return BOFS_FILE_OK;
                }
            }
        }
    }

    /* Insert into first empty indirect slot */
    for (uint32_t j = 0; j < BOFS_EXTENTS_PER_INDIRECT_BLOCK; j++) {
        if (!(ind->extents[j].flags & BOFS_EXTENT_FLAG_VALID)) {
            ind->extents[j].logical_block = logical_block;
            ind->extents[j].physical_block = physical_block;
            ind->extents[j].block_count = count;
            ind->extents[j].flags = flags | BOFS_EXTENT_FLAG_VALID;
            ind->checksum = bofs_crc32(ind, offsetof(bofs_indirect_block_t, checksum));
            if (!file->fs->dev->write(file->fs->dev, lba, sec_count, ind_buf)) {
                return BOFS_ERR_IO;
            }
            file->dirty = true;
            return BOFS_FILE_OK;
        }
    }

    return BOFS_ERR_EXTENT_LIMIT;
}

/* --------------------------------------------------------------------------
 * Public API: File Read
 * -------------------------------------------------------------------------- */
int bofs_file_read(bofs_file_t* file, uint64_t offset, void* buffer, uint64_t length, uint64_t* out_read) {
    if (!file || !file->is_open || !buffer || !out_read) return BOFS_ERR_INVALID_PARAM;
    if (offset + length < offset) return BOFS_ERR_OVERFLOW;

    if (offset >= file->inode.size_bytes || length == 0) {
        *out_read = 0;
        return BOFS_FILE_OK;
    }

    /* Clamp length to file size */
    if (offset + length > file->inode.size_bytes) {
        length = file->inode.size_bytes - offset;
    }

    uint8_t* out_ptr = (uint8_t*)buffer;
    uint64_t bytes_left = length;
    uint64_t cur_offset = offset;

    uint8_t blk_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));

    while (bytes_left > 0) {
        uint64_t lblk = cur_offset / BOFS_BLOCK_SIZE;
        uint32_t blk_off = (uint32_t)(cur_offset % BOFS_BLOCK_SIZE);
        uint32_t chunk = (uint32_t)(BOFS_BLOCK_SIZE - blk_off);
        if (chunk > bytes_left) chunk = (uint32_t)bytes_left;

        uint64_t phys_blk = 0;
        bool is_sparse = false;
        int bmap_res = bofs_file_bmap(file, lblk, &phys_blk, &is_sparse);

        if (bmap_res == BOFS_ERR_NOT_FOUND || is_sparse) {
            /* Sparse hole or unallocated space: return zeros */
            memset(out_ptr, 0, chunk);
        } else if (bmap_res == BOFS_FILE_OK) {
            uint64_t lba;
            uint32_t sec_count;
            if (!bofs_fs_block_to_lba(file->fs, phys_blk, &lba, &sec_count)) {
                return BOFS_ERR_OUT_OF_BOUNDS;
            }
            if (!file->fs->dev->read(file->fs->dev, lba, sec_count, blk_buf)) {
                return BOFS_ERR_IO;
            }
            memcpy(out_ptr, blk_buf + blk_off, chunk);
        } else {
            return bmap_res;
        }

        out_ptr += chunk;
        cur_offset += chunk;
        bytes_left -= chunk;
    }

    *out_read = length;
    return BOFS_FILE_OK;
}

/* --------------------------------------------------------------------------
 * Public API: File Write (Overwrite, Append, Growth, Partial-Block)
 * -------------------------------------------------------------------------- */
int bofs_file_write(bofs_file_t* file, uint64_t offset, const void* buffer, uint64_t length, uint64_t* out_written) {
    if (!file || !file->is_open || !buffer || !out_written) return BOFS_ERR_INVALID_PARAM;
    if (offset + length < offset) return BOFS_ERR_OVERFLOW;

    if (length == 0) {
        *out_written = 0;
        return BOFS_FILE_OK;
    }

    const uint8_t* in_ptr = (const uint8_t*)buffer;
    uint64_t bytes_left = length;
    uint64_t cur_offset = offset;

    uint8_t blk_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));

    while (bytes_left > 0) {
        uint64_t lblk = cur_offset / BOFS_BLOCK_SIZE;
        uint32_t blk_off = (uint32_t)(cur_offset % BOFS_BLOCK_SIZE);
        uint32_t chunk = (uint32_t)(BOFS_BLOCK_SIZE - blk_off);
        if (chunk > bytes_left) chunk = (uint32_t)bytes_left;

        uint64_t phys_blk = 0;
        bool is_sparse = false;
        int bmap_res = bofs_file_bmap(file, lblk, &phys_blk, &is_sparse);

        if (bmap_res == BOFS_ERR_NOT_FOUND || is_sparse) {
            /* Block does not exist or was sparse: allocate new block */
            uint64_t new_phys = 0;
            int alloc_res = bofs_alloc_block(file->fs->alloc, &new_phys);
            if (alloc_res != BOFS_ALLOC_OK) return alloc_res;

            file->inode.allocated_blocks++;

            /* Zero out new block buffer to prevent exposing stale data */
            memset(blk_buf, 0, BOFS_BLOCK_SIZE);
            memcpy(blk_buf + blk_off, in_ptr, chunk);

            uint64_t lba;
            uint32_t sec_count;
            if (!bofs_fs_block_to_lba(file->fs, new_phys, &lba, &sec_count)) {
                return BOFS_ERR_OUT_OF_BOUNDS;
            }
            if (!file->fs->dev->write(file->fs->dev, lba, sec_count, blk_buf)) {
                return BOFS_ERR_IO;
            }

            int app_res = bofs_file_append_extent(file, lblk, new_phys, 1, 0);
            if (app_res != BOFS_FILE_OK) return app_res;
        } else if (bmap_res == BOFS_FILE_OK) {
            /* Block exists on storage: read-modify-write for partial blocks */
            uint64_t lba;
            uint32_t sec_count;
            if (!bofs_fs_block_to_lba(file->fs, phys_blk, &lba, &sec_count)) {
                return BOFS_ERR_OUT_OF_BOUNDS;
            }

            if (chunk == BOFS_BLOCK_SIZE && blk_off == 0) {
                /* Full block overwrite */
                if (!file->fs->dev->write(file->fs->dev, lba, sec_count, (void*)in_ptr)) {
                    return BOFS_ERR_IO;
                }
            } else {
                /* Partial block write */
                if (!file->fs->dev->read(file->fs->dev, lba, sec_count, blk_buf)) {
                    return BOFS_ERR_IO;
                }
                memcpy(blk_buf + blk_off, in_ptr, chunk);
                if (!file->fs->dev->write(file->fs->dev, lba, sec_count, blk_buf)) {
                    return BOFS_ERR_IO;
                }
            }
        } else {
            return bmap_res;
        }

        in_ptr += chunk;
        cur_offset += chunk;
        bytes_left -= chunk;
    }

    if (offset + length > file->inode.size_bytes) {
        file->inode.size_bytes = offset + length;
    }

    file->dirty = true;
    int wr_res = bofs_inode_write(file->fs, &file->inode);
    if (wr_res != BOFS_FILE_OK) return wr_res;

    *out_written = length;
    return BOFS_FILE_OK;
}

/* --------------------------------------------------------------------------
 * Public API: Sparse Hole Writer
 * -------------------------------------------------------------------------- */
int bofs_file_write_sparse(bofs_file_t* file, uint64_t offset, uint64_t length) {
    if (!file || !file->is_open || length == 0) return BOFS_ERR_INVALID_PARAM;
    if (offset + length < offset) return BOFS_ERR_OVERFLOW;

    uint64_t start_lblk = offset / BOFS_BLOCK_SIZE;
    uint64_t end_lblk = (offset + length + BOFS_BLOCK_SIZE - 1) / BOFS_BLOCK_SIZE;
    uint32_t block_count = (uint32_t)(end_lblk - start_lblk);

    int app_res = bofs_file_append_extent(file, start_lblk, 0, block_count, BOFS_EXTENT_FLAG_SPARSE);
    if (app_res != BOFS_FILE_OK) return app_res;

    if (offset + length > file->inode.size_bytes) {
        file->inode.size_bytes = offset + length;
    }

    file->dirty = true;
    return bofs_inode_write(file->fs, &file->inode);
}

/* --------------------------------------------------------------------------
 * Public API: File Truncate / Shrink
 * -------------------------------------------------------------------------- */
int bofs_file_truncate(bofs_file_t* file, uint64_t new_size) {
    if (!file || !file->is_open) return BOFS_ERR_INVALID_PARAM;

    if (new_size == file->inode.size_bytes) {
        return BOFS_FILE_OK;
    }

    if (new_size > file->inode.size_bytes) {
        /* Growth via truncate creates a sparse gap */
        file->inode.size_bytes = new_size;
        file->dirty = true;
        return bofs_inode_write(file->fs, &file->inode);
    }

    /* Shrink / Truncate down */
    if (new_size == 0) {
        /* Free all direct extents */
        for (uint32_t i = 0; i < BOFS_INODE_DIRECT_EXTENTS; i++) {
            bofs_extent_t* ext = &file->inode.direct_extents[i];
            if (ext->flags & BOFS_EXTENT_FLAG_VALID) {
                if (!(ext->flags & BOFS_EXTENT_FLAG_SPARSE)) {
                    bofs_free_blocks(file->fs->alloc, ext->physical_block, ext->block_count);
                }
                memset(ext, 0, sizeof(bofs_extent_t));
            }
        }

        /* Free all indirect extents and the indirect block */
        if (file->inode.indirect_block != 0) {
            uint8_t ind_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
            uint64_t lba;
            uint32_t sec_count;
            if (bofs_fs_block_to_lba(file->fs, file->inode.indirect_block, &lba, &sec_count)) {
                if (file->fs->dev->read(file->fs->dev, lba, sec_count, ind_buf)) {
                    bofs_indirect_block_t* ind = (bofs_indirect_block_t*)ind_buf;
                    for (uint32_t j = 0; j < BOFS_EXTENTS_PER_INDIRECT_BLOCK; j++) {
                        bofs_extent_t* ext = &ind->extents[j];
                        if (ext->flags & BOFS_EXTENT_FLAG_VALID) {
                            if (!(ext->flags & BOFS_EXTENT_FLAG_SPARSE)) {
                                bofs_free_blocks(file->fs->alloc, ext->physical_block, ext->block_count);
                            }
                        }
                    }
                }
            }
            bofs_free_block(file->fs->alloc, file->inode.indirect_block);
            file->inode.indirect_block = 0;
        }

        file->inode.allocated_blocks = 0;
        file->inode.size_bytes = 0;
        file->dirty = true;
        return bofs_inode_write(file->fs, &file->inode);
    }

    /* Non-zero shrink */
    uint64_t cutoff_lblk = (new_size + BOFS_BLOCK_SIZE - 1) / BOFS_BLOCK_SIZE;

    /* Process direct extents */
    for (uint32_t i = 0; i < BOFS_INODE_DIRECT_EXTENTS; i++) {
        bofs_extent_t* ext = &file->inode.direct_extents[i];
        if (ext->flags & BOFS_EXTENT_FLAG_VALID) {
            if (ext->logical_block >= cutoff_lblk) {
                /* Entire extent is beyond cutoff: release */
                if (!(ext->flags & BOFS_EXTENT_FLAG_SPARSE)) {
                    bofs_free_blocks(file->fs->alloc, ext->physical_block, ext->block_count);
                    file->inode.allocated_blocks -= ext->block_count;
                }
                memset(ext, 0, sizeof(bofs_extent_t));
            } else if (ext->logical_block + ext->block_count > cutoff_lblk) {
                /* Extent straddles cutoff: split and release tail */
                uint32_t keep_cnt = (uint32_t)(cutoff_lblk - ext->logical_block);
                uint32_t free_cnt = ext->block_count - keep_cnt;
                if (!(ext->flags & BOFS_EXTENT_FLAG_SPARSE)) {
                    bofs_free_blocks(file->fs->alloc, ext->physical_block + keep_cnt, free_cnt);
                    file->inode.allocated_blocks -= free_cnt;
                }
                ext->block_count = keep_cnt;
            }
        }
    }

    /* Process indirect extents */
    if (file->inode.indirect_block != 0) {
        uint8_t ind_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
        uint64_t lba;
        uint32_t sec_count;
        if (bofs_fs_block_to_lba(file->fs, file->inode.indirect_block, &lba, &sec_count)) {
            if (file->fs->dev->read(file->fs->dev, lba, sec_count, ind_buf)) {
                bofs_indirect_block_t* ind = (bofs_indirect_block_t*)ind_buf;
                bool any_left = false;
                for (uint32_t j = 0; j < BOFS_EXTENTS_PER_INDIRECT_BLOCK; j++) {
                    bofs_extent_t* ext = &ind->extents[j];
                    if (ext->flags & BOFS_EXTENT_FLAG_VALID) {
                        if (ext->logical_block >= cutoff_lblk) {
                            if (!(ext->flags & BOFS_EXTENT_FLAG_SPARSE)) {
                                bofs_free_blocks(file->fs->alloc, ext->physical_block, ext->block_count);
                                file->inode.allocated_blocks -= ext->block_count;
                            }
                            memset(ext, 0, sizeof(bofs_extent_t));
                        } else if (ext->logical_block + ext->block_count > cutoff_lblk) {
                            uint32_t keep_cnt = (uint32_t)(cutoff_lblk - ext->logical_block);
                            uint32_t free_cnt = ext->block_count - keep_cnt;
                            if (!(ext->flags & BOFS_EXTENT_FLAG_SPARSE)) {
                                bofs_free_blocks(file->fs->alloc, ext->physical_block + keep_cnt, free_cnt);
                                file->inode.allocated_blocks -= free_cnt;
                            }
                            ext->block_count = keep_cnt;
                            any_left = true;
                        } else {
                            any_left = true;
                        }
                    }
                }

                if (!any_left) {
                    /* All indirect extents freed: release indirect block itself */
                    bofs_free_block(file->fs->alloc, file->inode.indirect_block);
                    file->inode.allocated_blocks--;
                    file->inode.indirect_block = 0;
                } else {
                    ind->checksum = bofs_crc32(ind, offsetof(bofs_indirect_block_t, checksum));
                    file->fs->dev->write(file->fs->dev, lba, sec_count, ind_buf);
                }
            }
        }
    }

    /* Zero out bytes beyond EOF in partial boundary block on disk */
    if (new_size % BOFS_BLOCK_SIZE != 0) {
        uint64_t last_lblk = new_size / BOFS_BLOCK_SIZE;
        uint64_t phys_blk = 0;
        bool is_sparse = false;
        if (bofs_file_bmap(file, last_lblk, &phys_blk, &is_sparse) == BOFS_FILE_OK && !is_sparse) {
            uint8_t blk_buf[BOFS_BLOCK_SIZE] __attribute__((aligned(64)));
            uint64_t lba;
            uint32_t sec_count;
            if (bofs_fs_block_to_lba(file->fs, phys_blk, &lba, &sec_count)) {
                if (file->fs->dev->read(file->fs->dev, lba, sec_count, blk_buf)) {
                    uint32_t tail_off = (uint32_t)(new_size % BOFS_BLOCK_SIZE);
                    memset(blk_buf + tail_off, 0, BOFS_BLOCK_SIZE - tail_off);
                    file->fs->dev->write(file->fs->dev, lba, sec_count, blk_buf);
                }
            }
        }
    }

    file->inode.size_bytes = new_size;
    file->dirty = true;
    return bofs_inode_write(file->fs, &file->inode);
}

/* --------------------------------------------------------------------------
 * Public API: File Deletion Lifecycle
 * -------------------------------------------------------------------------- */
int bofs_file_delete(bofs_file_system_t* fs, uint64_t inode_num, uint32_t generation) {
    if (!fs || inode_num < BOFS_FIRST_USER_INODE || inode_num >= fs->sb.total_inodes) {
        return BOFS_ERR_INVALID_PARAM;
    }

    bofs_file_t file;
    int op_res = bofs_file_open(fs, inode_num, generation, &file);
    if (op_res != BOFS_FILE_OK) return op_res;

    /* Release all data blocks and indirect blocks */
    int trunc_res = bofs_file_truncate(&file, 0);
    if (trunc_res != BOFS_FILE_OK) {
        bofs_file_close(&file);
        return trunc_res;
    }

    /* Increment generation on disk so stale handles are rejected */
    file.inode.generation++;
    file.inode.magic = 0; /* Mark unallocated */
    file.inode.size_bytes = 0;
    file.inode.allocated_blocks = 0;
    bofs_inode_write(fs, &file.inode);

    bofs_file_close(&file);

    /* Release Inode in Inode Bitmap */
    int free_res = bofs_inode_free(fs, inode_num);
    if (free_res != BOFS_FILE_OK) return free_res;

    return bofs_fs_flush(fs);
}
