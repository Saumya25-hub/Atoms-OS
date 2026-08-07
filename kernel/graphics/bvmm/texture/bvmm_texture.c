#include "bvmm_texture.h"
#include "../surface/bvmm_surface.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t btfe_registry_init(void);
extern bvmm_result_t btfe_registry_shutdown(void);
extern bvmm_result_t btfe_registry_register(btfe_texture_desc_t* desc, btfe_texture_id_t* out_id);
extern bvmm_result_t btfe_registry_lookup(btfe_texture_id_t id, btfe_texture_desc_t** out_desc);
extern bvmm_result_t btfe_registry_unregister(btfe_texture_id_t id);
extern btfe_swizzle_matrix_t btfe_get_swizzle_matrix(btfe_swizzle_mode_t mode);

static atoms_spinlock_t g_btfe_engine_lock;
static bool             g_btfe_active = false;
static btfe_diagnostics_t g_btfe_diag = {0};

bvmm_result_t btfe_init(void) {
    if (g_btfe_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_btfe_engine_lock, 0);
    memset(&g_btfe_diag, 0, sizeof(btfe_diagnostics_t));

    bvmm_result_t res = btfe_registry_init();
    if (res != BVMM_SUCCESS) return res;

    g_btfe_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_shutdown(void) {
    if (!g_btfe_active) return BVMM_ERR_NOT_INITIALIZED;

    btfe_registry_shutdown();
    g_btfe_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_texture_create(const btfe_texture_create_info_t* info, btfe_texture_id_t* out_texture_id) {
    if (!info || !out_texture_id || info->width == 0 || info->height == 0) {
        return BVMM_ERR_INVALID_ARGUMENT;
    }
    if (!g_btfe_active) return BVMM_ERR_NOT_INITIALIZED;

    *out_texture_id = BTFE_INVALID_TEXTURE_ID;

    /* 1. Calculate Mipchain & Footprint */
    btfe_mipchain_t mipchain;
    bvmm_result_t res = btfe_compute_mipchain(info->width, info->height, info->mip_levels, info->format, &mipchain);
    if (res != BVMM_SUCCESS) return res;

    /* 2. Allocate Texture Descriptor */
    btfe_texture_desc_t* desc = (btfe_texture_desc_t*)kmalloc(sizeof(btfe_texture_desc_t));
    if (!desc) return BVMM_ERR_OUT_OF_MEMORY;

    memset(desc, 0, sizeof(btfe_texture_desc_t));
    desc->canary_magic     = BTFE_TEXTURE_CANARY_MAGIC;
    desc->width            = info->width;
    desc->height           = info->height;
    desc->depth            = (info->depth > 0) ? info->depth : 1;
    desc->array_layers     = (info->array_layers > 0) ? info->array_layers : 1;
    desc->format           = info->format;
    desc->swizzle_mode     = info->swizzle;
    desc->swizzle_matrix   = btfe_get_swizzle_matrix(info->swizzle);
    desc->tile_mode        = info->tile_mode;
    desc->mipchain         = mipchain;
    desc->ref_count        = 1;
    desc->owner_pid        = info->owner_pid;

    /* 3. ABSOLUTE RULE: Texture MUST own exactly one BSME Surface */
    bvmm_surface_create_info_t surf_info = {
        .width            = info->width,
        .height           = info->height,
        .format           = BVMM_SURF_FMT_RGBA8888,
        .flags            = BVMM_SURF_FLAG_TEXTURE | BVMM_SURF_FLAG_GPU_ONLY,
        .preferred_domain = (info->preferred_domain != BVMM_DOMAIN_NONE) ? info->preferred_domain : BVMM_DOMAIN_VRAM,
        .lifetime         = BVMM_LIFETIME_PERSISTENT,
        .owner_pid        = info->owner_pid,
        .owner_module     = 505 /* BTFE Module Code */
    };

    bvmm_surface_id_t surf_id = BVMM_INVALID_SURFACE_ID;
    res = bvmm_surface_create(&surf_info, &surf_id);
    if (res != BVMM_SUCCESS || surf_id == BVMM_INVALID_SURFACE_ID) {
        kfree(desc);
        return res;
    }

    desc->surface_id = surf_id;

    /* 4. Register Texture in Global Registry */
    btfe_texture_id_t texture_id = BTFE_INVALID_TEXTURE_ID;
    res = btfe_registry_register(desc, &texture_id);
    if (res != BVMM_SUCCESS) {
        bvmm_surface_destroy(surf_id);
        kfree(desc);
        return res;
    }

    /* Update Telemetry */
    atoms_spin_lock(&g_btfe_engine_lock);
    g_btfe_diag.total_textures_created++;
    g_btfe_diag.alive_textures++;
    if (g_btfe_diag.alive_textures > g_btfe_diag.peak_textures) {
        g_btfe_diag.peak_textures = g_btfe_diag.alive_textures;
    }
    g_btfe_diag.total_texture_memory_bytes += mipchain.total_memory_bytes;
    g_btfe_diag.mip_memory_bytes += (mipchain.total_memory_bytes - mipchain.mips[0].size_bytes);
    if ((uint32_t)info->format <= 20) {
        g_btfe_diag.format_histogram[info->format]++;
    }
    if ((uint32_t)info->tile_mode <= 5) {
        g_btfe_diag.tile_histogram[info->tile_mode]++;
    }
    atoms_spin_unlock(&g_btfe_engine_lock);

    *out_texture_id = texture_id;
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_texture_destroy(btfe_texture_id_t texture_id) {
    if (texture_id == BTFE_INVALID_TEXTURE_ID) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_btfe_active) return BVMM_ERR_NOT_INITIALIZED;

    btfe_texture_desc_t* desc = NULL;
    bvmm_result_t res = btfe_registry_lookup(texture_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    /* Destroy backing BSME Surface */
    if (desc->surface_id != BVMM_INVALID_SURFACE_ID) {
        bvmm_surface_destroy(desc->surface_id);
        desc->surface_id = BVMM_INVALID_SURFACE_ID;
    }

    btfe_registry_unregister(texture_id);

    atoms_spin_lock(&g_btfe_engine_lock);
    if (g_btfe_diag.alive_textures > 0) g_btfe_diag.alive_textures--;
    g_btfe_diag.destroyed_textures++;
    if (g_btfe_diag.total_texture_memory_bytes >= desc->mipchain.total_memory_bytes) {
        g_btfe_diag.total_texture_memory_bytes -= desc->mipchain.total_memory_bytes;
    }
    atoms_spin_unlock(&g_btfe_engine_lock);

    kfree(desc);
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_texture_lookup(btfe_texture_id_t texture_id, btfe_texture_desc_t** out_desc) {
    return btfe_registry_lookup(texture_id, out_desc);
}

bvmm_result_t btfe_texture_ref_inc(btfe_texture_id_t texture_id) {
    btfe_texture_desc_t* desc = NULL;
    bvmm_result_t res = btfe_registry_lookup(texture_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    desc->ref_count++;
    return BVMM_SUCCESS;
}

bvmm_result_t btfe_texture_ref_dec(btfe_texture_id_t texture_id) {
    btfe_texture_desc_t* desc = NULL;
    bvmm_result_t res = btfe_registry_lookup(texture_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (desc->ref_count > 0) desc->ref_count--;

    if (desc->ref_count == 0) {
        return btfe_texture_destroy(texture_id);
    }

    return BVMM_SUCCESS;
}

bvmm_result_t btfe_texture_upload(btfe_texture_id_t texture_id, uint32_t mip_level, const void* src_pixels, size_t src_size_bytes) {
    if (texture_id == BTFE_INVALID_TEXTURE_ID || !src_pixels || src_size_bytes == 0) {
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    btfe_texture_desc_t* desc = NULL;
    bvmm_result_t res = btfe_registry_lookup(texture_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    if (mip_level >= desc->mipchain.mip_count) {
        g_btfe_diag.failed_uploads++;
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    btfe_mip_desc_t* mip = &desc->mipchain.mips[mip_level];
    if (src_size_bytes < mip->size_bytes) {
        g_btfe_diag.failed_uploads++;
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    /* Lock BSME surface BAR mapping to copy CPU pixels into GPU texture buffer */
    void* cpu_surface_ptr = NULL;
    res = bvmm_surface_lock(desc->surface_id, &cpu_surface_ptr);
    if (res != BVMM_SUCCESS || !cpu_surface_ptr) {
        g_btfe_diag.failed_uploads++;
        return res;
    }

    uint8_t* dst_mip_ptr = (uint8_t*)cpu_surface_ptr + mip->offset_bytes;
    memcpy(dst_mip_ptr, src_pixels, mip->size_bytes);

    bvmm_surface_unlock(desc->surface_id);

    atoms_spin_lock(&g_btfe_engine_lock);
    g_btfe_diag.upload_count++;
    g_btfe_diag.upload_bytes_total += mip->size_bytes;
    atoms_spin_unlock(&g_btfe_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t btfe_texture_create_view(btfe_texture_id_t texture_id, btfe_view_type_t view_type, btfe_texture_view_desc_t* out_view) {
    if (texture_id == BTFE_INVALID_TEXTURE_ID || !out_view) return BVMM_ERR_INVALID_ARGUMENT;

    btfe_texture_desc_t* desc = NULL;
    bvmm_result_t res = btfe_registry_lookup(texture_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    out_view->base_texture_id   = texture_id;
    out_view->view_type         = view_type;
    out_view->base_mip_level    = 0;
    out_view->mip_level_count   = desc->mipchain.mip_count;
    out_view->base_array_layer  = 0;
    out_view->array_layer_count = desc->array_layers;
    out_view->view_format       = desc->format;

    return BVMM_SUCCESS;
}

bvmm_result_t btfe_get_diagnostics(btfe_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_btfe_engine_lock);
    *out_diag = g_btfe_diag;
    atoms_spin_unlock(&g_btfe_engine_lock);
    return BVMM_SUCCESS;
}
