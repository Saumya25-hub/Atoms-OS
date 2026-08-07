#include "bvmm_pools.h"
#include "../handles/bvmm_handle_table.h"
#include "../diagnostics/bvmm_stats.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static bvmm_pool_engine_t g_pool_engine;

bvmm_result_t bvmm_pools_init(void) {
    if (g_pool_engine.active) return BVMM_ERR_ALREADY_INITIALIZED;

    memset(&g_pool_engine, 0, sizeof(bvmm_pool_engine_t));

    g_pool_engine.parent_heap_vram = bvmm_heap_get_primary(BVMM_DOMAIN_VRAM, BVMM_POOL_LARGE);
    g_pool_engine.parent_heap_gtt  = bvmm_heap_get_primary(BVMM_DOMAIN_GTT, BVMM_POOL_LARGE);

    /* 1. Small Pool (<64KB) */
    g_pool_engine.stats[BVMM_POOL_SMALL].pool_type        = BVMM_POOL_SMALL;
    g_pool_engine.stats[BVMM_POOL_SMALL].name             = "Small Pool";
    g_pool_engine.stats[BVMM_POOL_SMALL].domain           = BVMM_DOMAIN_VRAM;
    g_pool_engine.stats[BVMM_POOL_SMALL].min_object_size  = 16;
    g_pool_engine.stats[BVMM_POOL_SMALL].max_object_size  = 64 * 1024;
    g_pool_engine.stats[BVMM_POOL_SMALL].alignment_policy = 16;

    /* 2. Medium Pool (64KB - 4MB) */
    g_pool_engine.stats[BVMM_POOL_MEDIUM].pool_type       = BVMM_POOL_MEDIUM;
    g_pool_engine.stats[BVMM_POOL_MEDIUM].name            = "Medium Pool";
    g_pool_engine.stats[BVMM_POOL_MEDIUM].domain          = BVMM_DOMAIN_VRAM;
    g_pool_engine.stats[BVMM_POOL_MEDIUM].min_object_size = 64 * 1024;
    g_pool_engine.stats[BVMM_POOL_MEDIUM].max_object_size = 4 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_MEDIUM].alignment_policy= 256;

    /* 3. Large Pool (4MB - 64MB) */
    g_pool_engine.stats[BVMM_POOL_LARGE].pool_type        = BVMM_POOL_LARGE;
    g_pool_engine.stats[BVMM_POOL_LARGE].name             = "Large Pool";
    g_pool_engine.stats[BVMM_POOL_LARGE].domain           = BVMM_DOMAIN_VRAM;
    g_pool_engine.stats[BVMM_POOL_LARGE].min_object_size  = 4 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_LARGE].max_object_size  = 64 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_LARGE].alignment_policy = 65536;

    /* 4. Huge Pool (>64MB) */
    g_pool_engine.stats[BVMM_POOL_HUGE].pool_type         = BVMM_POOL_HUGE;
    g_pool_engine.stats[BVMM_POOL_HUGE].name              = "Huge Pool";
    g_pool_engine.stats[BVMM_POOL_HUGE].domain            = BVMM_DOMAIN_VRAM;
    g_pool_engine.stats[BVMM_POOL_HUGE].min_object_size   = 64 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_HUGE].max_object_size   = 1024 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_HUGE].alignment_policy  = 65536;

    /* 5. Transient Pool (Per-frame ring) */
    g_pool_engine.stats[BVMM_POOL_TRANSIENT].pool_type    = BVMM_POOL_TRANSIENT;
    g_pool_engine.stats[BVMM_POOL_TRANSIENT].name         = "Transient Pool";
    g_pool_engine.stats[BVMM_POOL_TRANSIENT].domain       = BVMM_DOMAIN_GTT;
    g_pool_engine.stats[BVMM_POOL_TRANSIENT].min_object_size = 16;
    g_pool_engine.stats[BVMM_POOL_TRANSIENT].max_object_size = 16 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_TRANSIENT].alignment_policy= 16;

    /* 6. Upload Pool (CPU->GPU Write-Combining) */
    g_pool_engine.stats[BVMM_POOL_UPLOAD].pool_type       = BVMM_POOL_UPLOAD;
    g_pool_engine.stats[BVMM_POOL_UPLOAD].name            = "Upload Pool";
    g_pool_engine.stats[BVMM_POOL_UPLOAD].domain          = BVMM_DOMAIN_UPLOAD;
    g_pool_engine.stats[BVMM_POOL_UPLOAD].min_object_size = 16;
    g_pool_engine.stats[BVMM_POOL_UPLOAD].max_object_size = 64 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_UPLOAD].alignment_policy= 256;

    /* 7. Readback Pool (GPU->CPU Transfer) */
    g_pool_engine.stats[BVMM_POOL_READBACK].pool_type     = BVMM_POOL_READBACK;
    g_pool_engine.stats[BVMM_POOL_READBACK].name          = "Readback Pool";
    g_pool_engine.stats[BVMM_POOL_READBACK].domain        = BVMM_DOMAIN_READBACK;
    g_pool_engine.stats[BVMM_POOL_READBACK].min_object_size= 16;
    g_pool_engine.stats[BVMM_POOL_READBACK].max_object_size= 64 * 1024 * 1024;
    g_pool_engine.stats[BVMM_POOL_READBACK].alignment_policy= 256;

    g_pool_engine.active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_pools_shutdown(void) {
    if (!g_pool_engine.active) return BVMM_ERR_NOT_INITIALIZED;
    memset(&g_pool_engine, 0, sizeof(bvmm_pool_engine_t));
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_pool_route(const bvmm_alloc_info_t* info,
                              bvmm_pool_type_t* out_pool_type,
                              bvmm_heap_t** out_heap,
                              size_t* out_alignment) {
    if (!info || !out_pool_type || !out_heap || !out_alignment) {
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    bvmm_pool_type_t routed_pool = BVMM_POOL_MEDIUM;
    size_t size = info->size_bytes;
    uint32_t flags = info->alloc_flags;

    /* Automatic Pool Selection Logic */
    if (flags & BVMM_POOL_FLAG_TRANSIENT) {
        routed_pool = BVMM_POOL_TRANSIENT;
    } else if (flags & BVMM_POOL_FLAG_UPLOAD) {
        routed_pool = BVMM_POOL_UPLOAD;
    } else if (flags & BVMM_POOL_FLAG_READBACK) {
        routed_pool = BVMM_POOL_READBACK;
    } else if (size < 64 * 1024) {
        routed_pool = BVMM_POOL_SMALL;
    } else if (size < 4 * 1024 * 1024) {
        routed_pool = BVMM_POOL_MEDIUM;
    } else if (size < 64 * 1024 * 1024) {
        routed_pool = BVMM_POOL_LARGE;
    } else {
        routed_pool = BVMM_POOL_HUGE;
    }

    bvmm_domain_t target_domain = g_pool_engine.stats[routed_pool].domain;
    if (info->preferred_domain != BVMM_DOMAIN_NONE) {
        target_domain = info->preferred_domain;
    }

    *out_pool_type = routed_pool;
    *out_heap      = bvmm_heap_get_primary(target_domain, routed_pool);
    
    size_t req_align = info->alignment_bytes;
    size_t pool_align = g_pool_engine.stats[routed_pool].alignment_policy;
    *out_alignment = (req_align > pool_align) ? req_align : pool_align;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_pool_allocate(bvmm_pool_type_t pool_type,
                                 const bvmm_alloc_info_t* info,
                                 bvmm_handle_t* out_handle) {
    if (!info || !out_handle || pool_type >= BVMM_POOL_COUNT) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_pool_engine.active) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_pool_type_t routed_pool;
    bvmm_heap_t* heap;
    size_t alignment;

    bvmm_result_t res = bvmm_pool_route(info, &routed_pool, &heap, &alignment);
    if (res != BVMM_SUCCESS || !heap) {
        g_pool_engine.stats[pool_type].failed_allocations++;
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    uint64_t vram_offset = 0;
    res = bvmm_heap_allocate_range(heap, info->size_bytes, alignment, BVMM_POLICY_BEST_FIT, &vram_offset);
    if (res != BVMM_SUCCESS) {
        g_pool_engine.stats[routed_pool].failed_allocations++;
        return res;
    }

    bvmm_allocation_t* alloc = (bvmm_allocation_t*)kmalloc(sizeof(bvmm_allocation_t));
    if (!alloc) {
        bvmm_heap_free_range(heap, vram_offset, info->size_bytes);
        g_pool_engine.stats[routed_pool].failed_allocations++;
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    memset(alloc, 0, sizeof(bvmm_allocation_t));
    alloc->vram_offset      = vram_offset;
    alloc->cpu_virtual_addr = (void*)(uintptr_t)vram_offset;
    alloc->size_bytes       = info->size_bytes;
    alloc->alignment_bytes  = alignment;
    alloc->current_domain   = heap->domain;
    alloc->pool_type        = routed_pool;
    alloc->state            = BVMM_STATE_RESIDENT;
    alloc->alloc_flags      = info->alloc_flags;
    alloc->cpu_refcount     = 1;
    alloc->gpu_refcount     = 0;
    alloc->owner_pid        = info->owner_pid;
    alloc->width            = info->width;
    alloc->height           = info->height;
    alloc->stride_bytes     = info->stride_bytes;
    alloc->format_fourcc    = info->format_fourcc;

    res = bvmm_handle_table_insert(alloc, out_handle);
    if (res != BVMM_SUCCESS) {
        bvmm_heap_free_range(heap, vram_offset, info->size_bytes);
        kfree(alloc);
        g_pool_engine.stats[routed_pool].failed_allocations++;
        return res;
    }

    /* Update per-pool statistics */
    bvmm_pool_telemetry_t* t = &g_pool_engine.stats[routed_pool];
    t->total_allocated_bytes += info->size_bytes;
    t->active_objects++;
    if (t->total_allocated_bytes > t->peak_bytes) {
        t->peak_bytes = t->total_allocated_bytes;
    }
    if (info->size_bytes > t->largest_alloc_bytes) {
        t->largest_alloc_bytes = info->size_bytes;
    }

    bvmm_stats_record_alloc(heap->domain, routed_pool, info->size_bytes);
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_pool_free(bvmm_handle_t handle) {
    if (handle == BVMM_INVALID_HANDLE) return BVMM_ERR_INVALID_HANDLE;
    if (!g_pool_engine.active) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_allocation_t* alloc = NULL;
    bvmm_result_t res = bvmm_handle_table_lookup(handle, &alloc);
    if (res != BVMM_SUCCESS || !alloc) return res;

    if (alloc->cpu_refcount > 0) alloc->cpu_refcount--;
    if (alloc->cpu_refcount > 0 || alloc->gpu_refcount > 0) return BVMM_SUCCESS;

    bvmm_heap_t* heap = bvmm_heap_get_primary(alloc->current_domain, alloc->pool_type);
    if (heap) {
        bvmm_heap_free_range(heap, alloc->vram_offset, alloc->size_bytes);
    }

    bvmm_pool_type_t ptype = alloc->pool_type;
    if (ptype < BVMM_POOL_COUNT) {
        bvmm_pool_telemetry_t* t = &g_pool_engine.stats[ptype];
        if (t->total_allocated_bytes >= alloc->size_bytes) {
            t->total_allocated_bytes -= alloc->size_bytes;
        }
        if (t->active_objects > 0) t->active_objects--;
    }

    bvmm_stats_record_free(alloc->current_domain, alloc->pool_type, alloc->size_bytes);
    bvmm_handle_table_remove(handle);
    kfree(alloc);

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_pool_transient_reset(void) {
    if (!g_pool_engine.active) return BVMM_ERR_NOT_INITIALIZED;

    /* O(1) Bulk Reset for Frame-Lifetime Transient Pool */
    bvmm_pool_telemetry_t* t = &g_pool_engine.stats[BVMM_POOL_TRANSIENT];
    t->total_allocated_bytes = 0;
    t->active_objects        = 0;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_pool_get_telemetry(bvmm_pool_type_t pool_type, bvmm_pool_telemetry_t* out_telemetry) {
    if (pool_type >= BVMM_POOL_COUNT || !out_telemetry) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_pool_engine.active) return BVMM_ERR_NOT_INITIALIZED;

    *out_telemetry = g_pool_engine.stats[pool_type];
    return BVMM_SUCCESS;
}
