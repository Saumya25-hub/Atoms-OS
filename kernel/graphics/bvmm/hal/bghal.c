#include "bghal.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/sync/spinlock.h"
#include "kernel/core/lib/include/string.h"

extern bvmm_result_t bghal_registry_init(void);
extern bvmm_result_t bghal_registry_shutdown(void);
extern bvmm_result_t bghal_registry_register_gpu(bghal_gpu_device_t* gpu, bghal_gpu_id_t* out_id);
extern bvmm_result_t bghal_registry_get_primary(bghal_gpu_device_t** out_gpu);
extern uint32_t bghal_registry_get_active_count(void);

extern const bghal_backend_vtbl_t* bghal_get_intel_backend(void);
extern const bghal_backend_vtbl_t* bghal_get_amd_backend(void);
extern const bghal_backend_vtbl_t* bghal_get_nvidia_backend(void);
extern const bghal_backend_vtbl_t* bghal_get_virtio_backend(void);
extern const bghal_backend_vtbl_t* bghal_get_vmware_backend(void);
extern const bghal_backend_vtbl_t* bghal_get_swrender_backend(void);

static atoms_spinlock_t   g_hal_engine_lock;
static bool               g_hal_active = false;
static bghal_diagnostics_t g_hal_diag = {0};

bvmm_result_t bghal_init(void) {
    if (g_hal_active) return BVMM_ERR_ALREADY_INITIALIZED;

    atoms_spinlock_init(&g_hal_engine_lock, 0);
    memset(&g_hal_diag, 0, sizeof(bghal_diagnostics_t));

    bvmm_result_t res = bghal_registry_init();
    if (res != BVMM_SUCCESS) return res;

    uint32_t count = 0;
    res = bghal_discover_gpus(&count);
    if (res != BVMM_SUCCESS) {
        bghal_registry_shutdown();
        return res;
    }

    g_hal_active = true;
    return BVMM_SUCCESS;
}

bvmm_result_t bghal_shutdown(void) {
    if (!g_hal_active) return BVMM_ERR_NOT_INITIALIZED;

    bghal_registry_shutdown();
    g_hal_active = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bghal_discover_gpus(uint32_t* out_gpu_count) {
    if (!out_gpu_count) return BVMM_ERR_INVALID_ARGUMENT;

    /* Discover & Register Primary GPU (Intel Iris Xe / Software Renderer) */
    bghal_gpu_device_t* gpu = (bghal_gpu_device_t*)kmalloc(sizeof(bghal_gpu_device_t));
    if (!gpu) return BVMM_ERR_OUT_OF_MEMORY;

    memset(gpu, 0, sizeof(bghal_gpu_device_t));
    gpu->canary_magic = BGHAL_CANARY_MAGIC;
    gpu->vtbl         = bghal_get_intel_backend();

    if (gpu->vtbl && gpu->vtbl->init) {
        gpu->vtbl->init(gpu);
    }

    bghal_gpu_id_t id = BGHAL_INVALID_GPU_ID;
    bvmm_result_t res = bghal_registry_register_gpu(gpu, &id);
    if (res != BVMM_SUCCESS) {
        kfree(gpu);
        return res;
    }

    atoms_spin_lock(&g_hal_engine_lock);
    g_hal_diag.total_gpus_detected++;
    g_hal_diag.active_gpus++;
    atoms_spin_unlock(&g_hal_engine_lock);

    *out_gpu_count = bghal_registry_get_active_count();
    return BVMM_SUCCESS;
}

bvmm_result_t bghal_get_primary_gpu(bghal_gpu_device_t** out_gpu) {
    return bghal_registry_get_primary(out_gpu);
}

bvmm_result_t bghal_dma_copy(bghal_gpu_device_t* gpu, const bghal_dma_req_t* req) {
    if (!gpu || !req) return BVMM_ERR_INVALID_ARGUMENT;
    if (gpu->vtbl && gpu->vtbl->dma_copy) {
        gpu->vtbl->dma_copy(gpu, req);
    }

    atoms_spin_lock(&g_hal_engine_lock);
    g_hal_diag.total_dma_copies++;
    g_hal_diag.total_bytes_dma_transferred += req->size_bytes;
    atoms_spin_unlock(&g_hal_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bghal_page_table_map(bghal_gpu_device_t* gpu, const bghal_page_table_t* pt) {
    if (!gpu || !pt) return BVMM_ERR_INVALID_ARGUMENT;
    if (gpu->vtbl && gpu->vtbl->page_table_update) {
        gpu->vtbl->page_table_update(gpu, pt);
    }

    atoms_spin_lock(&g_hal_engine_lock);
    g_hal_diag.total_page_table_updates++;
    atoms_spin_unlock(&g_hal_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bghal_cache_flush_and_tlb_invalidate(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    if (gpu->vtbl && gpu->vtbl->cache_flush) {
        gpu->vtbl->cache_flush(gpu);
    }
    if (gpu->vtbl && gpu->vtbl->tlb_invalidate) {
        gpu->vtbl->tlb_invalidate(gpu);
    }

    atoms_spin_lock(&g_hal_engine_lock);
    g_hal_diag.cache_flushes++;
    g_hal_diag.tlb_invalidations++;
    atoms_spin_unlock(&g_hal_engine_lock);

    return BVMM_SUCCESS;
}

bvmm_result_t bghal_query_capabilities(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps) {
    if (!gpu || !out_caps) return BVMM_ERR_INVALID_ARGUMENT;
    if (gpu->vtbl && gpu->vtbl->query_capabilities) {
        return gpu->vtbl->query_capabilities(gpu, out_caps);
    }

    memset(out_caps, 0, sizeof(bghal_capabilities_t));
    out_caps->max_texture_dim = 8192;
    out_caps->max_single_alloc_bytes = 512 * 1024 * 1024ULL;
    out_caps->max_dma_transfer_bytes = 32 * 1024 * 1024ULL;
    return BVMM_SUCCESS;
}

bvmm_result_t bghal_get_diagnostics(bghal_diagnostics_t* out_diag) {
    if (!out_diag) return BVMM_ERR_INVALID_ARGUMENT;
    atoms_spin_lock(&g_hal_engine_lock);
    *out_diag = g_hal_diag;
    atoms_spin_unlock(&g_hal_engine_lock);
    return BVMM_SUCCESS;
}
