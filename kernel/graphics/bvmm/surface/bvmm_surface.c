#include "bvmm_surface.h"
#include "../include/bvmm.h"
#include "../pools/bvmm_pools.h"
#include "../diagnostics/bvmm_stats.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t bvmm_surface_registry_init(void);
extern bvmm_result_t bvmm_surface_registry_shutdown(void);
extern bvmm_result_t bvmm_surface_registry_register(bvmm_surface_desc_t* desc, bvmm_surface_id_t* out_id);
extern bvmm_result_t bvmm_surface_registry_lookup(bvmm_surface_id_t id, bvmm_surface_desc_t** out_desc);
extern bvmm_result_t bvmm_surface_registry_unregister(bvmm_surface_id_t id);

static atoms_spinlock_t g_surface_engine_lock;
static bool             g_surface_engine_active = false;
static bvmm_surface_stats_t g_bsme_stats = {0};

/* Bytes-per-pixel calculator for surface formats */
static uint32_t bvmm_surface_bpp(bvmm_surface_format_t fmt) {
    switch (fmt) {
        case BVMM_SURF_FMT_R8:        return 1;
        case BVMM_SURF_FMT_RGB565:
        case BVMM_SURF_FMT_R16:       return 2;
        case BVMM_SURF_FMT_RGBA8888:
        case BVMM_SURF_FMT_ARGB8888:
        case BVMM_SURF_FMT_XRGB8888:
        case BVMM_SURF_FMT_R32F:
        case BVMM_SURF_FMT_DEPTH24:
        case BVMM_SURF_FMT_DEPTH32:   return 4;
        default:                      return 4;
    }
}

bvmm_result_t bvmm_surface_engine_init(void) {
    if (g_surface_engine_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_surface_engine_lock, 0);
    memset(&g_bsme_stats, 0, sizeof(bvmm_surface_stats_t));

    bvmm_result_t res = bvmm_surface_registry_init();
    if (res != BVMM_SUCCESS) return res;

    g_surface_engine_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_engine_shutdown(void) {
    if (!g_surface_engine_active) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_surface_registry_shutdown();
    g_surface_engine_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_create(const bvmm_surface_create_info_t* info, bvmm_surface_id_t* out_surface_id) {
    if (!info || !out_surface_id || info->width == 0 || info->height == 0) {
        return BVMM_ERR_INVALID_ARGUMENT;
    }
    if (!g_surface_engine_active) return BVMM_ERR_NOT_INITIALIZED;

    *out_surface_id = BVMM_INVALID_SURFACE_ID;

    uint32_t bpp = bvmm_surface_bpp(info->format);
    uint32_t stride = (info->width * bpp + 15) & ~15U;
    size_t total_size = (size_t)stride * info->height;

    /* 1. State: CREATED */
    bvmm_surface_desc_t* desc = (bvmm_surface_desc_t*)kmalloc(sizeof(bvmm_surface_desc_t));
    if (!desc) {
        g_bsme_stats.allocation_failures++;
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    memset(desc, 0, sizeof(bvmm_surface_desc_t));
    desc->canary_magic  = BVMM_SURFACE_CANARY_MAGIC;
    desc->width         = info->width;
    desc->height        = info->height;
    desc->stride_bytes  = stride;
    desc->format        = info->format;
    desc->size_bytes    = total_size;
    desc->flags         = info->flags;
    desc->owner_pid     = info->owner_pid;
    desc->owner_module  = info->owner_module;
    desc->ref_count     = 1;
    desc->state         = BVMM_SURF_STATE_CREATED;

    /* 2. Request memory allocation through PMPE */
    bvmm_alloc_info_t alloc_info = {
        .size_bytes       = total_size,
        .alignment_bytes  = 256,
        .preferred_domain = (info->preferred_domain != BVMM_DOMAIN_NONE) ? info->preferred_domain : BVMM_DOMAIN_VRAM,
        .alloc_flags      = info->flags,
        .owner_pid        = info->owner_pid,
        .width            = info->width,
        .height           = info->height,
        .stride_bytes     = stride
    };

    bvmm_handle_t alloc_h = BVMM_INVALID_HANDLE;
    bvmm_result_t res = bvmm_pool_allocate(BVMM_POOL_MEDIUM, &alloc_info, &alloc_h);
    if (res != BVMM_SUCCESS || alloc_h == BVMM_INVALID_HANDLE) {
        kfree(desc);
        g_bsme_stats.allocation_failures++;
        return res;
    }

    desc->alloc_handle = alloc_h;
    desc->state        = BVMM_SURF_STATE_ALLOCATED;

    /* 3. Register surface in global registry */
    bvmm_surface_id_t surface_id = BVMM_INVALID_SURFACE_ID;
    res = bvmm_surface_registry_register(desc, &surface_id);
    if (res != BVMM_SUCCESS) {
        bvmm_pool_free(alloc_h);
        kfree(desc);
        g_bsme_stats.allocation_failures++;
        return res;
    }

    desc->state = BVMM_SURF_STATE_READY;

    /* Update BSME telemetry */
    atoms_spin_lock(&g_surface_engine_lock);
    g_bsme_stats.total_surfaces_created++;
    g_bsme_stats.alive_surfaces++;
    if (g_bsme_stats.alive_surfaces > g_bsme_stats.peak_surfaces) {
        g_bsme_stats.peak_surfaces = g_bsme_stats.alive_surfaces;
    }
    g_bsme_stats.total_memory_bytes += total_size;
    if (total_size > g_bsme_stats.largest_surface_bytes) {
        g_bsme_stats.largest_surface_bytes = total_size;
    }
    atoms_spin_unlock(&g_surface_engine_lock);

    *out_surface_id = surface_id;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_destroy(bvmm_surface_id_t surface_id) {
    if (surface_id == BVMM_INVALID_SURFACE_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_surface_engine_active) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_surface_desc_t* desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(surface_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->lock_info.lock_count > 0) {
        g_bsme_stats.validation_failures++;
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    desc->state = BVMM_SURF_STATE_RELEASED;

    if (desc->alloc_handle != BVMM_INVALID_HANDLE) {
        bvmm_pool_free(desc->alloc_handle);
        desc->alloc_handle = BVMM_INVALID_HANDLE;
    }

    bvmm_surface_registry_unregister(surface_id);

    atoms_spin_lock(&g_surface_engine_lock);
    if (g_bsme_stats.alive_surfaces > 0) g_bsme_stats.alive_surfaces--;
    g_bsme_stats.destroyed_surfaces++;
    if (g_bsme_stats.total_memory_bytes >= desc->size_bytes) {
        g_bsme_stats.total_memory_bytes -= desc->size_bytes;
    }
    atoms_spin_unlock(&g_surface_engine_lock);

    kfree(desc);
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_lookup(bvmm_surface_id_t surface_id, bvmm_surface_desc_t** out_desc) {
    return bvmm_surface_registry_lookup(surface_id, out_desc);
}

bvmm_result_t bvmm_surface_clone(bvmm_surface_id_t src_surface_id, bvmm_surface_id_t* out_cloned_id) {
    if (src_surface_id == BVMM_INVALID_SURFACE_ID || !out_cloned_id) return BVMM_ERR_INVALID_ARGUMENT;

    bvmm_surface_desc_t* src_desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(src_surface_id, &src_desc);
    if (res != BVMM_SUCCESS || !src_desc) return res;

    bvmm_surface_create_info_t clone_info = {
        .width            = src_desc->width,
        .height           = src_desc->height,
        .format           = src_desc->format,
        .flags            = src_desc->flags,
        .owner_pid        = src_desc->owner_pid,
        .owner_module     = src_desc->owner_module
    };

    return bvmm_surface_create(&clone_info, out_cloned_id);
}

bvmm_result_t bvmm_surface_resize(bvmm_surface_id_t surface_id, uint32_t new_width, uint32_t new_height) {
    if (surface_id == BVMM_INVALID_SURFACE_ID || new_width == 0 || new_height == 0) {
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    bvmm_surface_desc_t* desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(surface_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->lock_info.lock_count > 0) return BVMM_ERR_INVALID_ARGUMENT;

    uint32_t bpp = bvmm_surface_bpp(desc->format);
    uint32_t stride = (new_width * bpp + 15) & ~15U;
    size_t new_size = (size_t)stride * new_height;

    bvmm_alloc_info_t alloc_info = {
        .size_bytes       = new_size,
        .alignment_bytes  = 256,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .alloc_flags      = desc->flags,
        .owner_pid        = desc->owner_pid,
        .width            = new_width,
        .height           = new_height,
        .stride_bytes     = stride
    };

    bvmm_handle_t new_handle = BVMM_INVALID_HANDLE;
    res = bvmm_pool_allocate(BVMM_POOL_MEDIUM, &alloc_info, &new_handle);
    if (res != BVMM_SUCCESS || new_handle == BVMM_INVALID_HANDLE) {
        g_bsme_stats.allocation_failures++;
        return res;
    }

    if (desc->alloc_handle != BVMM_INVALID_HANDLE) {
        bvmm_pool_free(desc->alloc_handle);
    }

    desc->alloc_handle = new_handle;
    desc->width        = new_width;
    desc->height       = new_height;
    desc->stride_bytes = stride;
    desc->size_bytes   = new_size;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_ref_inc(bvmm_surface_id_t surface_id) {
    bvmm_surface_desc_t* desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(surface_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    desc->ref_count++;
    desc->state = BVMM_SURF_STATE_REFERENCED;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_ref_dec(bvmm_surface_id_t surface_id) {
    bvmm_surface_desc_t* desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(surface_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->ref_count > 0) {
        desc->ref_count--;
    }

    if (desc->ref_count == 0) {
        return bvmm_surface_destroy(surface_id);
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_lock(bvmm_surface_id_t surface_id, void** out_cpu_ptr) {
    if (!out_cpu_ptr) return BVMM_ERR_INVALID_ARGUMENT;

    bvmm_surface_desc_t* desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(surface_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    void* cpu_ptr = NULL;
    res = bvmm_map(desc->alloc_handle, &cpu_ptr);
    if (res != BVMM_SUCCESS) return res;

    desc->lock_info.lock_count++;
    desc->lock_info.mapped_cpu_ptr = cpu_ptr;
    desc->state = BVMM_SURF_STATE_LOCKED;

    *out_cpu_ptr = cpu_ptr;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_unlock(bvmm_surface_id_t surface_id) {
    bvmm_surface_desc_t* desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(surface_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->lock_info.lock_count == 0) {
        g_bsme_stats.validation_failures++;
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    desc->lock_info.lock_count--;
    if (desc->lock_info.lock_count == 0) {
        bvmm_unmap(desc->alloc_handle);
        desc->lock_info.mapped_cpu_ptr = NULL;
        desc->state = BVMM_SURF_STATE_UNLOCKED;
    }

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_surface_trylock(bvmm_surface_id_t surface_id, void** out_cpu_ptr) {
    bvmm_surface_desc_t* desc = NULL;
    bvmm_result_t res = bvmm_surface_registry_lookup(surface_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->lock_info.lock_count > 0) {
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    return bvmm_surface_lock(surface_id, out_cpu_ptr);
}

bvmm_result_t bvmm_surface_get_stats(bvmm_surface_stats_t* out_stats) {
    if (!out_stats) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_surface_engine_lock);
    *out_stats = g_bsme_stats;
    atoms_spin_unlock(&g_surface_engine_lock);
    return BVMM_SUCCESS;
}
