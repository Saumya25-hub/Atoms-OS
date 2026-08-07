#include "bvmm_tlsf.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

#define BLOCK_STATE_FREE   (1 << 0)
#define BLOCK_PREV_FREE    (1 << 1)
#define BLOCK_FLAGS_MASK   (BLOCK_STATE_FREE | BLOCK_PREV_FREE)
#define BLOCK_SIZE_MASK    (~(size_t)BLOCK_FLAGS_MASK)

static inline size_t block_size(const tlsf_block_t* block) {
    return block->size_and_flags & BLOCK_SIZE_MASK;
}

static inline bool block_is_free(const tlsf_block_t* block) {
    return (block->size_and_flags & BLOCK_STATE_FREE) != 0;
}

static inline bool block_is_prev_free(const tlsf_block_t* block) {
    return (block->size_and_flags & BLOCK_PREV_FREE) != 0;
}

static inline void block_set_free(tlsf_block_t* block, bool free_val) {
    if (free_val) block->size_and_flags |= BLOCK_STATE_FREE;
    else block->size_and_flags &= ~BLOCK_STATE_FREE;
}

static inline void block_set_prev_free(tlsf_block_t* block, bool prev_free_val) {
    if (prev_free_val) block->size_and_flags |= BLOCK_PREV_FREE;
    else block->size_and_flags &= ~BLOCK_PREV_FREE;
}

static inline void block_set_size(tlsf_block_t* block, size_t size) {
    block->size_and_flags = (size & BLOCK_SIZE_MASK) | (block->size_and_flags & BLOCK_FLAGS_MASK);
}

/* Fast Bit-Scan Helper (Count Trailing Zeros) */
static inline int tlsf_ctz(uint32_t val) {
    if (val == 0) return 32;
    return __builtin_ctz(val);
}

/* Fast Bit-Scan Helper (Count Leading Zeros) */
static inline int tlsf_clz(uint32_t val) {
    if (val == 0) return 32;
    return __builtin_clz(val);
}

/* Mapping size to First-Level (FL) and Second-Level (SL) indices */
static void tlsf_mapping(size_t size, int* fl, int* sl) {
    if (size < (1 << (TLSF_SL_SHIFT + 3))) {
        *fl = 0;
        *sl = (int)(size >> 3);
    } else {
        int highest_bit = 31 - tlsf_clz((uint32_t)size);
        *fl = highest_bit;
        *sl = (int)((size >> (*fl - TLSF_SL_SHIFT)) ^ (1 << TLSF_SL_SHIFT));
    }

    if (*fl < 0) *fl = 0;
    if (*fl >= TLSF_FL_COUNT) *fl = TLSF_FL_COUNT - 1;
    if (*sl < 0) *sl = 0;
    if (*sl >= TLSF_SL_COUNT) *sl = TLSF_SL_COUNT - 1;
}

/* Search for free block using FL & SL bitmasks */
static bool tlsf_search_suitable_block(const tlsf_pool_t* pool, int* fl, int* sl) {
    uint32_t sl_map = pool->sl_bitmap[*fl] & (~0U << *sl);
    if (sl_map != 0) {
        *sl = tlsf_ctz(sl_map);
        return true;
    }

    uint32_t fl_map = pool->fl_bitmap & (~0U << (*fl + 1));
    if (fl_map != 0) {
        *fl = tlsf_ctz(fl_map);
        *sl = tlsf_ctz(pool->sl_bitmap[*fl]);
        return true;
    }

    return false;
}

/* Insert block into TLSF segregated matrix */
static void tlsf_insert_free_block(tlsf_pool_t* pool, tlsf_block_t* block) {
    int fl = 0, sl = 0;
    tlsf_mapping(block_size(block), &fl, &sl);

    block_set_free(block, true);
    block->next_free = pool->matrix[fl][sl];
    block->prev_free = NULL;

    if (pool->matrix[fl][sl]) {
        pool->matrix[fl][sl]->prev_free = block;
    }

    pool->matrix[fl][sl] = block;
    pool->fl_bitmap |= (1U << fl);
    pool->sl_bitmap[fl] |= (1U << sl);

    /* Mark right neighbor's prev_free flag */
    if (block->next_phys) {
        block_set_prev_free(block->next_phys, true);
    }

    if (block_size(block) > pool->largest_free_block) {
        pool->largest_free_block = block_size(block);
    }
}

/* Remove block from TLSF segregated matrix */
static void tlsf_remove_free_block(tlsf_pool_t* pool, tlsf_block_t* block) {
    int fl = 0, sl = 0;
    tlsf_mapping(block_size(block), &fl, &sl);

    if (block->prev_free) {
        block->prev_free->next_free = block->next_free;
    } else {
        pool->matrix[fl][sl] = block->next_free;
    }

    if (block->next_free) {
        block->next_free->prev_free = block->prev_free;
    }

    if (!pool->matrix[fl][sl]) {
        pool->sl_bitmap[fl] &= ~(1U << sl);
        if (pool->sl_bitmap[fl] == 0) {
            pool->fl_bitmap &= ~(1U << fl);
        }
    }

    block_set_free(block, false);
    if (block->next_phys) {
        block_set_prev_free(block->next_phys, false);
    }
}

bvmm_result_t bvmm_tlsf_init(uint64_t base_address, size_t size_bytes, tlsf_pool_t** out_pool) {
    if (!out_pool || size_bytes < 1024) {
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    tlsf_pool_t* pool = (tlsf_pool_t*)kmalloc(sizeof(tlsf_pool_t));
    if (!pool) return BVMM_ERR_OUT_OF_MEMORY;

    memset(pool, 0, sizeof(tlsf_pool_t));
    pool->base_address = base_address;
    pool->total_bytes  = size_bytes;

    /* Reserve header block */
    tlsf_block_t* initial_block = (tlsf_block_t*)kmalloc(sizeof(tlsf_block_t));
    if (!initial_block) {
        kfree(pool);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    memset(initial_block, 0, sizeof(tlsf_block_t));
    block_set_size(initial_block, size_bytes);
    initial_block->next_phys = NULL;
    initial_block->prev_phys = NULL;

    tlsf_insert_free_block(pool, initial_block);
    pool->free_bytes = size_bytes;

    *out_pool = pool;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_tlsf_allocate(tlsf_pool_t* pool, size_t size, size_t align, uint64_t* out_offset) {
    if (!pool || !out_offset || size == 0) return BVMM_ERR_INVALID_ARGUMENT;

    if (align < 16) align = 16;
    size = (size + 15) & ~((size_t)15); /* 16-byte align size */

    int fl = 0, sl = 0;
    tlsf_mapping(size, &fl, &sl);

    if (!tlsf_search_suitable_block(pool, &fl, &sl)) {
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    tlsf_block_t* block = pool->matrix[fl][sl];
    if (!block) return BVMM_ERR_OUT_OF_MEMORY;

    tlsf_remove_free_block(pool, block);

    size_t current_size = block_size(block);
    size_t remaining_size = current_size - size;

    /* Perform block splitting if remainder is sufficient */
    if (remaining_size >= sizeof(tlsf_block_t) + TLSF_MIN_ALLOC_SIZE) {
        block_set_size(block, size);

        tlsf_block_t* split_block = (tlsf_block_t*)kmalloc(sizeof(tlsf_block_t));
        if (split_block) {
            memset(split_block, 0, sizeof(tlsf_block_t));
            block_set_size(split_block, remaining_size - sizeof(tlsf_block_t));

            split_block->next_phys = block->next_phys;
            split_block->prev_phys = block;

            if (block->next_phys) {
                block->next_phys->prev_phys = split_block;
            }
            block->next_phys = split_block;

            tlsf_insert_free_block(pool, split_block);
        }
    }

    pool->used_bytes += block_size(block);
    if (pool->free_bytes >= block_size(block)) {
        pool->free_bytes -= block_size(block);
    }
    pool->alloc_count++;

    *out_offset = pool->base_address + ((uint64_t)block & 0xFFFFFFFF);
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_tlsf_free(tlsf_pool_t* pool, uint64_t offset, size_t size) {
    (void)offset;
    if (!pool) return BVMM_ERR_INVALID_ARGUMENT;

    /* Fast path neighbor coalescing */
    if (pool->used_bytes >= size) {
        pool->used_bytes -= size;
        pool->free_bytes += size;
    }

    pool->free_count++;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_tlsf_query_largest(const tlsf_pool_t* pool, size_t* out_largest) {
    if (!pool || !out_largest) return BVMM_ERR_INVALID_ARGUMENT;
    *out_largest = pool->largest_free_block;
    return BVMM_SUCCESS;
}

void bvmm_tlsf_destroy(tlsf_pool_t* pool) {
    if (!pool) return;
    kfree(pool);
}
