#include "bvmm_heap.h"
#include "bvmm_tlsf.h"
#include "bvmm_range.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static bvmm_heap_t* g_heaps[BVMM_MAX_HEAPS] = {0};
static uint32_t     g_heap_count            = 0;
static bool         g_heap_subsystem_active = false;

bvmm_result_t bvmm_heap_subsystem_init(void) {
    if (g_heap_subsystem_active) return BVMM_ERR_ALREADY_INITIALIZED;

    memset(g_heaps, 0, sizeof(g_heaps));
    g_heap_count = 0;
    g_heap_subsystem_active = true;

    /* Initialize Primary VRAM Heap (Default 256MB VRAM aperture space) */
    bvmm_heap_t* vram_heap = NULL;
    bvmm_result_t res = bvmm_heap_create(0, BVMM_DOMAIN_VRAM, 0xE0000000ULL, 256 * 1024 * 1024ULL, &vram_heap);
    if (res != BVMM_SUCCESS) return res;

    /* Initialize Primary GTT Heap (Default 128MB System GTT aperture) */
    bvmm_heap_t* gtt_heap = NULL;
    res = bvmm_heap_create(1, BVMM_DOMAIN_GTT, 0xF0000000ULL, 128 * 1024 * 1024ULL, &gtt_heap);
    if (res != BVMM_SUCCESS) return res;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_heap_subsystem_shutdown(void) {
    if (!g_heap_subsystem_active) return BVMM_ERR_NOT_INITIALIZED;

    for (uint32_t i = 0; i < g_heap_count; i++) {
        if (g_heaps[i]) {
            bvmm_heap_destroy(g_heaps[i]);
            g_heaps[i] = NULL;
        }
    }

    g_heap_count = 0;
    g_heap_subsystem_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_heap_create(uint32_t heap_id,
                               bvmm_domain_t domain,
                               uint64_t base_address,
                               size_t size_bytes,
                               bvmm_heap_t** out_heap) {
    if (!out_heap || size_bytes == 0) return BVMM_ERR_INVALID_ARGUMENT;
    if (g_heap_count >= BVMM_MAX_HEAPS) return BVMM_ERR_OUT_OF_MEMORY;

    bvmm_heap_t* heap = (bvmm_heap_t*)kmalloc(sizeof(bvmm_heap_t));
    if (!heap) return BVMM_ERR_OUT_OF_MEMORY;

    memset(heap, 0, sizeof(bvmm_heap_t));
    heap->heap_id                  = heap_id;
    heap->domain                   = domain;
    heap->base_address             = base_address;
    heap->total_size_bytes         = size_bytes;
    heap->free_size_bytes          = size_bytes;
    heap->largest_free_block_bytes = size_bytes;

    /* Initialize underlying Spatial Range Manager */
    bvmm_range_manager_t* range_mgr = NULL;
    bvmm_result_t res = bvmm_range_init(base_address, size_bytes, &range_mgr);
    if (res != BVMM_SUCCESS) {
        kfree(heap);
        return res;
    }

    heap->private_data = (void*)range_mgr;
    g_heaps[g_heap_count++] = heap;

    *out_heap = heap;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_heap_destroy(bvmm_heap_t* heap) {
    if (!heap) return BVMM_ERR_INVALID_ARGUMENT;

    if (heap->private_data) {
        bvmm_range_destroy((bvmm_range_manager_t*)heap->private_data);
        heap->private_data = NULL;
    }

    kfree(heap);
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_heap_allocate_range(bvmm_heap_t* heap,
                                        size_t size_bytes,
                                        size_t alignment,
                                        bvmm_alloc_policy_t policy,
                                        uint64_t* out_offset) {
    if (!heap || !out_offset || size_bytes == 0) return BVMM_ERR_INVALID_ARGUMENT;
    if (!heap->private_data) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_range_manager_t* mgr = (bvmm_range_manager_t*)heap->private_data;
    bvmm_result_t res = bvmm_range_alloc(mgr, size_bytes, alignment, policy, out_offset);
    if (res != BVMM_SUCCESS) return res;

    heap->used_size_bytes           = mgr->used_bytes;
    heap->free_size_bytes           = mgr->free_bytes;
    heap->largest_free_block_bytes = mgr->largest_free_block;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_heap_free_range(bvmm_heap_t* heap, uint64_t offset, size_t size_bytes) {
    if (!heap) return BVMM_ERR_INVALID_ARGUMENT;
    if (!heap->private_data) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_range_manager_t* mgr = (bvmm_range_manager_t*)heap->private_data;
    bvmm_result_t res = bvmm_range_free(mgr, offset, size_bytes);
    if (res != BVMM_SUCCESS) return res;

    heap->used_size_bytes           = mgr->used_bytes;
    heap->free_size_bytes           = mgr->free_bytes;
    heap->largest_free_block_bytes = mgr->largest_free_block;

    return BVMM_SUCCESS;
}

bvmm_heap_t* bvmm_heap_get_primary(bvmm_domain_t domain, bvmm_pool_type_t pool_type) {
    (void)pool_type;
    for (uint32_t i = 0; i < g_heap_count; i++) {
        if (g_heaps[i] && (g_heaps[i]->domain & domain)) {
            return g_heaps[i];
        }
    }
    return (g_heap_count > 0) ? g_heaps[0] : NULL;
}

bool bvmm_heap_validate(const bvmm_heap_t* heap) {
    if (!heap || !heap->private_data) return false;
    const bvmm_range_manager_t* mgr = (const bvmm_range_manager_t*)heap->private_data;
    return (mgr->head != NULL && (mgr->used_bytes + mgr->free_bytes == mgr->total_bytes));
}
