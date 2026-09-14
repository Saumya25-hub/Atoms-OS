#include "kernel/vfs/bofs/include/bofs_alloc.h"
#include "kernel/vfs/bofs/include/bofs_validator.h"
#include "kernel/core/lib/include/string.h"

/* --------------------------------------------------------------------------
 * Internal Helper: Block to Sector Translation
 * -------------------------------------------------------------------------- */
static inline bool bofs_block_to_lba(const bofs_allocator_t* alloc, uint64_t block_idx, uint64_t* out_lba, uint32_t* out_sec_count) {
    if (!alloc || !alloc->dev || alloc->sb.sector_size == 0) return false;
    uint32_t spb = BOFS_BLOCK_SIZE / (uint32_t)alloc->sb.sector_size;
    if (spb == 0) return false;

    if (block_idx > (UINT64_MAX / spb)) return false; /* Overflow guard */

    *out_lba = block_idx * spb;
    *out_sec_count = spb;
    return true;
}

/* --------------------------------------------------------------------------
 * Internal Helper: Bitmap Block Cache Manager
 * -------------------------------------------------------------------------- */
static int bofs_load_bitmap_block(bofs_allocator_t* alloc, uint64_t bmp_block_idx) {
    if (!alloc || !alloc->dev) return BOFS_ERR_INVALID_PARAM;
    if (alloc->cached_bitmap_block == bmp_block_idx) return BOFS_ALLOC_OK;

    /* Flush dirty cached block before eviction */
    if (alloc->cached_bitmap_dirty && alloc->cached_bitmap_block != (uint64_t)-1) {
        int flush_res = bofs_allocator_flush(alloc);
        if (flush_res != BOFS_ALLOC_OK) return flush_res;
    }

    uint64_t lba;
    uint32_t count;
    if (!bofs_block_to_lba(alloc, bmp_block_idx, &lba, &count)) return BOFS_ERR_OUT_OF_BOUNDS;

    if (!alloc->dev->read(alloc->dev, lba, count, alloc->cached_bitmap)) {
        return BOFS_ERR_IO;
    }

    alloc->cached_bitmap_block = bmp_block_idx;
    alloc->cached_bitmap_dirty = false;
    return BOFS_ALLOC_OK;
}

/* --------------------------------------------------------------------------
 * Internal Helper: Check If Block Intersects Active In-Memory Reservation
 * -------------------------------------------------------------------------- */
static bool bofs_is_block_reserved(const bofs_allocator_t* alloc, uint64_t block_idx) {
    for (int i = 0; i < BOFS_MAX_RESERVATIONS; i++) {
        if (alloc->reservations[i].active) {
            uint64_t start = alloc->reservations[i].start_block;
            uint64_t end = start + alloc->reservations[i].count;
            if (block_idx >= start && block_idx < end) {
                return true;
            }
        }
    }
    return false;
}

/* --------------------------------------------------------------------------
 * Bitmap Bit Operations (0 = Free, 1 = Allocated)
 * -------------------------------------------------------------------------- */
static inline bool bofs_test_bit_in_block(const uint8_t* block_buf, uint32_t local_bit) {
    return (block_buf[local_bit >> 3] & (1U << (local_bit & 7))) != 0;
}

static inline void bofs_set_bit_in_block(uint8_t* block_buf, uint32_t local_bit) {
    block_buf[local_bit >> 3] |= (uint8_t)(1U << (local_bit & 7));
}

static inline void bofs_clear_bit_in_block(uint8_t* block_buf, uint32_t local_bit) {
    block_buf[local_bit >> 3] &= (uint8_t)~(1U << (local_bit & 7));
}

/* --------------------------------------------------------------------------
 * Public API: bofs_allocator_init
 * -------------------------------------------------------------------------- */
int bofs_allocator_init(bofs_allocator_t* alloc, BlockDevice* dev) {
    if (!alloc || !dev) return BOFS_ERR_INVALID_PARAM;
    memset(alloc, 0, sizeof(bofs_allocator_t));
    alloc->dev = dev;

    /* Sector size verification */
    if (dev->sector_size != 512 && dev->sector_size != 4096) return BOFS_ERR_INVALID_PARAM;
    uint32_t spb = BOFS_BLOCK_SIZE / (uint32_t)dev->sector_size;

    /* Read Superblock at Block 0 */
    if (!dev->read(dev, 0, spb, &alloc->sb)) {
        return BOFS_ERR_IO;
    }

    /* Validate Superblock */
    uint64_t total_dev_blocks = dev->sector_count / spb;
    int sb_res = bofs_validate_superblock(&alloc->sb, total_dev_blocks);
    if (sb_res != BOFS_VALID_OK) {
        return BOFS_ERR_CORRUPT_BITMAP;
    }

    alloc->data_pool_start = alloc->sb.data_pool_start_block;
    alloc->data_pool_end = alloc->sb.total_blocks - 1;
    alloc->last_alloc_cursor = alloc->data_pool_start;

    alloc->cached_bitmap_block = (uint64_t)-1;
    alloc->cached_bitmap_dirty = false;

    for (int i = 0; i < BOFS_MAX_RESERVATIONS; i++) {
        alloc->reservations[i].active = false;
    }
    alloc->next_reservation_id = 1;

    /* Initial Free-Space Count */
    alloc->free_blocks_count = bofs_count_free_blocks(alloc);

    return BOFS_ALLOC_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_allocator_flush
 * -------------------------------------------------------------------------- */
int bofs_allocator_flush(bofs_allocator_t* alloc) {
    if (!alloc || !alloc->dev) return BOFS_ERR_INVALID_PARAM;

    if (alloc->cached_bitmap_dirty && alloc->cached_bitmap_block != (uint64_t)-1) {
        uint64_t lba;
        uint32_t count;
        if (!bofs_block_to_lba(alloc, alloc->cached_bitmap_block, &lba, &count)) {
            return BOFS_ERR_OUT_OF_BOUNDS;
        }

        if (!alloc->dev->write(alloc->dev, lba, count, alloc->cached_bitmap)) {
            return BOFS_ERR_IO;
        }
        alloc->cached_bitmap_dirty = false;
    }

    if (alloc->dev->flush) {
        if (!alloc->dev->flush(alloc->dev)) {
            return BOFS_ERR_IO;
        }
    }

    return BOFS_ALLOC_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_alloc_block (Single Block Allocation)
 * -------------------------------------------------------------------------- */
int bofs_alloc_block(bofs_allocator_t* alloc, uint64_t* out_block) {
    if (!alloc || !out_block) return BOFS_ERR_INVALID_PARAM;
    if (alloc->free_blocks_count == 0) return BOFS_ERR_OUT_OF_SPACE;

    uint64_t total_data = alloc->data_pool_end - alloc->data_pool_start + 1;
    uint64_t cur = alloc->last_alloc_cursor;

    for (uint64_t scanned = 0; scanned < total_data; scanned++) {
        if (cur > alloc->data_pool_end) {
            cur = alloc->data_pool_start;
        }

        /* Skip if under active reservation */
        if (bofs_is_block_reserved(alloc, cur)) {
            cur++;
            continue;
        }

        /* Calculate bitmap block & bit */
        uint64_t bmp_blk_offset = cur / 32768ULL;
        uint64_t bmp_disk_block = alloc->sb.block_bitmap_start_block + bmp_blk_offset;
        uint32_t local_bit = (uint32_t)(cur % 32768ULL);

        int load_res = bofs_load_bitmap_block(alloc, bmp_disk_block);
        if (load_res != BOFS_ALLOC_OK) return load_res;

        /* Test if free (0) */
        if (!bofs_test_bit_in_block(alloc->cached_bitmap, local_bit)) {
            /* Proof of free verified: flip bit to 1 */
            bofs_set_bit_in_block(alloc->cached_bitmap, local_bit);
            alloc->cached_bitmap_dirty = true;

            int flush_res = bofs_allocator_flush(alloc);
            if (flush_res != BOFS_ALLOC_OK) {
                /* Rollback in-memory mutation upon I/O failure */
                bofs_clear_bit_in_block(alloc->cached_bitmap, local_bit);
                alloc->cached_bitmap_dirty = false;
                return flush_res;
            }

            alloc->free_blocks_count--;
            alloc->last_alloc_cursor = cur + 1;
            *out_block = cur;
            return BOFS_ALLOC_OK;
        }

        cur++;
    }

    return BOFS_ERR_OUT_OF_SPACE;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_alloc_blocks_contiguous
 * -------------------------------------------------------------------------- */
int bofs_alloc_blocks_contiguous(bofs_allocator_t* alloc, uint32_t count, uint64_t* out_start_block) {
    if (!alloc || !out_start_block || count == 0) return BOFS_ERR_INVALID_PARAM;
    if (alloc->free_blocks_count < count) return BOFS_ERR_OUT_OF_SPACE;

    uint64_t start_candidate = alloc->data_pool_start;
    uint32_t matched = 0;

    for (uint64_t blk = alloc->data_pool_start; blk <= alloc->data_pool_end; blk++) {
        if (bofs_is_block_reserved(alloc, blk)) {
            matched = 0;
            start_candidate = blk + 1;
            continue;
        }

        uint64_t bmp_blk_offset = blk / 32768ULL;
        uint64_t bmp_disk_block = alloc->sb.block_bitmap_start_block + bmp_blk_offset;
        uint32_t local_bit = (uint32_t)(blk % 32768ULL);

        int load_res = bofs_load_bitmap_block(alloc, bmp_disk_block);
        if (load_res != BOFS_ALLOC_OK) return load_res;

        if (!bofs_test_bit_in_block(alloc->cached_bitmap, local_bit)) {
            if (matched == 0) start_candidate = blk;
            matched++;
            if (matched == count) {
                /* Full contiguous run verified: commit bits */
                for (uint64_t set_blk = start_candidate; set_blk < start_candidate + count; set_blk++) {
                    uint64_t s_bmp_blk = alloc->sb.block_bitmap_start_block + (set_blk / 32768ULL);
                    uint32_t s_bit = (uint32_t)(set_blk % 32768ULL);

                    bofs_load_bitmap_block(alloc, s_bmp_blk);
                    bofs_set_bit_in_block(alloc->cached_bitmap, s_bit);
                    alloc->cached_bitmap_dirty = true;
                }

                int flush_res = bofs_allocator_flush(alloc);
                if (flush_res != BOFS_ALLOC_OK) return flush_res;

                alloc->free_blocks_count -= count;
                alloc->last_alloc_cursor = start_candidate + count;
                *out_start_block = start_candidate;
                return BOFS_ALLOC_OK;
            }
        } else {
            matched = 0;
            start_candidate = blk + 1;
        }
    }

    return BOFS_ERR_OUT_OF_SPACE;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_alloc_blocks_fragmented
 * -------------------------------------------------------------------------- */
int bofs_alloc_blocks_fragmented(bofs_allocator_t* alloc, uint32_t count, bofs_alloc_extent_list_t* out_list) {
    if (!alloc || !out_list || count == 0) return BOFS_ERR_INVALID_PARAM;
    if (alloc->free_blocks_count < count) return BOFS_ERR_OUT_OF_SPACE;

    memset(out_list, 0, sizeof(bofs_alloc_extent_list_t));
    uint32_t remaining = count;

    while (remaining > 0) {
        if (out_list->count >= BOFS_MAX_ALLOC_EXTENTS) {
            /* Extent list exhausted before satisfying request: rollback */
            for (uint32_t e = 0; e < out_list->count; e++) {
                bofs_free_blocks(alloc, out_list->extents[e].start_block, out_list->extents[e].count);
            }
            memset(out_list, 0, sizeof(bofs_alloc_extent_list_t));
            return BOFS_ERR_OUT_OF_SPACE;
        }

        uint64_t allocated_blk = 0;
        int res = bofs_alloc_block(alloc, &allocated_blk);
        if (res != BOFS_ALLOC_OK) {
            /* Rollback all previously acquired extents */
            for (uint32_t e = 0; e < out_list->count; e++) {
                bofs_free_blocks(alloc, out_list->extents[e].start_block, out_list->extents[e].count);
            }
            memset(out_list, 0, sizeof(bofs_alloc_extent_list_t));
            return res;
        }

        /* Check if contiguous with previous extent in list */
        if (out_list->count > 0) {
            bofs_alloc_extent_t* prev = &out_list->extents[out_list->count - 1];
            if (prev->start_block + prev->count == allocated_blk) {
                prev->count++;
                out_list->total_blocks++;
                remaining--;
                continue;
            }
        }

        /* Add new extent */
        bofs_alloc_extent_t* cur = &out_list->extents[out_list->count++];
        cur->start_block = allocated_blk;
        cur->count = 1;
        out_list->total_blocks++;
        remaining--;
    }

    return BOFS_ALLOC_OK;
}

/* --------------------------------------------------------------------------
 * Public API: Reservation Model (Reserve != Commit)
 * -------------------------------------------------------------------------- */
int bofs_alloc_reserve_blocks(bofs_allocator_t* alloc, uint32_t count, uint32_t* out_res_id, uint64_t* out_start_block) {
    if (!alloc || !out_res_id || !out_start_block || count == 0) return BOFS_ERR_INVALID_PARAM;
    if (alloc->free_blocks_count < count) return BOFS_ERR_OUT_OF_SPACE;

    /* Find free slot in reservation table */
    int slot = -1;
    for (int i = 0; i < BOFS_MAX_RESERVATIONS; i++) {
        if (!alloc->reservations[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return BOFS_ERR_RESERVATION_FULL;

    /* Scan for contiguous free and unreserved blocks */
    uint64_t start_candidate = alloc->data_pool_start;
    uint32_t matched = 0;

    for (uint64_t blk = alloc->data_pool_start; blk <= alloc->data_pool_end; blk++) {
        if (bofs_is_block_reserved(alloc, blk)) {
            matched = 0;
            start_candidate = blk + 1;
            continue;
        }

        uint64_t bmp_blk_offset = blk / 32768ULL;
        uint64_t bmp_disk_block = alloc->sb.block_bitmap_start_block + bmp_blk_offset;
        uint32_t local_bit = (uint32_t)(blk % 32768ULL);

        int load_res = bofs_load_bitmap_block(alloc, bmp_disk_block);
        if (load_res != BOFS_ALLOC_OK) return load_res;

        if (!bofs_test_bit_in_block(alloc->cached_bitmap, local_bit)) {
            if (matched == 0) start_candidate = blk;
            matched++;
            if (matched == count) {
                /* Reservation candidate acquired (NO BITMAP MODIFICATION YET) */
                alloc->reservations[slot].id = alloc->next_reservation_id++;
                alloc->reservations[slot].active = true;
                alloc->reservations[slot].start_block = start_candidate;
                alloc->reservations[slot].count = count;

                *out_res_id = alloc->reservations[slot].id;
                *out_start_block = start_candidate;
                return BOFS_ALLOC_OK;
            }
        } else {
            matched = 0;
            start_candidate = blk + 1;
        }
    }

    return BOFS_ERR_OUT_OF_SPACE;
}

int bofs_alloc_commit_reservation(bofs_allocator_t* alloc, uint32_t res_id) {
    if (!alloc || res_id == 0) return BOFS_ERR_INVALID_PARAM;

    for (int i = 0; i < BOFS_MAX_RESERVATIONS; i++) {
        if (alloc->reservations[i].active && alloc->reservations[i].id == res_id) {
            uint64_t start = alloc->reservations[i].start_block;
            uint32_t count = alloc->reservations[i].count;

            for (uint64_t blk = start; blk < start + count; blk++) {
                uint64_t bmp_blk = alloc->sb.block_bitmap_start_block + (blk / 32768ULL);
                uint32_t bit = (uint32_t)(blk % 32768ULL);

                bofs_load_bitmap_block(alloc, bmp_blk);
                bofs_set_bit_in_block(alloc->cached_bitmap, bit);
                alloc->cached_bitmap_dirty = true;
            }

            int flush_res = bofs_allocator_flush(alloc);
            if (flush_res != BOFS_ALLOC_OK) return flush_res;

            alloc->free_blocks_count -= count;
            alloc->reservations[i].active = false;
            return BOFS_ALLOC_OK;
        }
    }

    return BOFS_ERR_RESERVATION_NOT_FOUND;
}

int bofs_alloc_rollback_reservation(bofs_allocator_t* alloc, uint32_t res_id) {
    if (!alloc || res_id == 0) return BOFS_ERR_INVALID_PARAM;

    for (int i = 0; i < BOFS_MAX_RESERVATIONS; i++) {
        if (alloc->reservations[i].active && alloc->reservations[i].id == res_id) {
            alloc->reservations[i].active = false;
            /* Zero disk modifications */
            return BOFS_ALLOC_OK;
        }
    }

    return BOFS_ERR_RESERVATION_NOT_FOUND;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_free_block (Safe Block Freeing)
 * -------------------------------------------------------------------------- */
int bofs_free_block(bofs_allocator_t* alloc, uint64_t block_num) {
    if (!alloc) return BOFS_ERR_INVALID_PARAM;

    /* Reject metadata regions */
    if (block_num < alloc->data_pool_start) {
        return BOFS_ERR_METADATA_PROTECTED;
    }
    if (block_num > alloc->data_pool_end) {
        return BOFS_ERR_OUT_OF_BOUNDS;
    }

    uint64_t bmp_blk_offset = block_num / 32768ULL;
    uint64_t bmp_disk_block = alloc->sb.block_bitmap_start_block + bmp_blk_offset;
    uint32_t local_bit = (uint32_t)(block_num % 32768ULL);

    int load_res = bofs_load_bitmap_block(alloc, bmp_disk_block);
    if (load_res != BOFS_ALLOC_OK) return load_res;

    /* Double-Free Check: must currently be 1 (Allocated) */
    if (!bofs_test_bit_in_block(alloc->cached_bitmap, local_bit)) {
        return BOFS_ERR_DOUBLE_FREE;
    }

    bofs_clear_bit_in_block(alloc->cached_bitmap, local_bit);
    alloc->cached_bitmap_dirty = true;

    int flush_res = bofs_allocator_flush(alloc);
    if (flush_res != BOFS_ALLOC_OK) {
        /* Rollback in-memory mutation upon I/O failure */
        bofs_set_bit_in_block(alloc->cached_bitmap, local_bit);
        alloc->cached_bitmap_dirty = false;
        return flush_res;
    }

    alloc->free_blocks_count++;
    return BOFS_ALLOC_OK;
}

int bofs_free_blocks(bofs_allocator_t* alloc, uint64_t start_block, uint32_t count) {
    if (!alloc || count == 0) return BOFS_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < count; i++) {
        int res = bofs_free_block(alloc, start_block + i);
        if (res != BOFS_ALLOC_OK) return res;
    }

    return BOFS_ALLOC_OK;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_count_free_blocks (Authoritative Bitmap Scanner)
 * -------------------------------------------------------------------------- */
uint64_t bofs_count_free_blocks(bofs_allocator_t* alloc) {
    if (!alloc || !alloc->dev) return 0;
    uint64_t free_count = 0;

    for (uint64_t blk = alloc->data_pool_start; blk <= alloc->data_pool_end; blk++) {
        uint64_t bmp_blk_offset = blk / 32768ULL;
        uint64_t bmp_disk_block = alloc->sb.block_bitmap_start_block + bmp_blk_offset;
        uint32_t local_bit = (uint32_t)(blk % 32768ULL);

        if (bofs_load_bitmap_block(alloc, bmp_disk_block) != BOFS_ALLOC_OK) return 0;

        if (!bofs_test_bit_in_block(alloc->cached_bitmap, local_bit)) {
            free_count++;
        }
    }

    return free_count;
}

/* --------------------------------------------------------------------------
 * Public API: bofs_validate_bitmap
 * -------------------------------------------------------------------------- */
int bofs_validate_bitmap(bofs_allocator_t* alloc) {
    if (!alloc || !alloc->dev) return BOFS_ERR_INVALID_PARAM;

    /* Verify all metadata blocks before data_pool_start are marked 1 (ALLOCATED/PROTECTED) */
    for (uint64_t meta_blk = 0; meta_blk < alloc->data_pool_start; meta_blk++) {
        uint64_t bmp_blk_offset = meta_blk / 32768ULL;
        uint64_t bmp_disk_block = alloc->sb.block_bitmap_start_block + bmp_blk_offset;
        uint32_t local_bit = (uint32_t)(meta_blk % 32768ULL);

        int load_res = bofs_load_bitmap_block(alloc, bmp_disk_block);
        if (load_res != BOFS_ALLOC_OK) return load_res;

        if (!bofs_test_bit_in_block(alloc->cached_bitmap, local_bit)) {
            /* Metadata block is improperly marked free! */
            return BOFS_ERR_CORRUPT_BITMAP;
        }
    }

    return BOFS_ALLOC_OK;
}
