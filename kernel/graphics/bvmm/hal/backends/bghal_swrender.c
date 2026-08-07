#include "../bghal.h"

static bvmm_result_t swrender_init(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->vendor_id = BGHAL_VENDOR_SOFTWARE;
    gpu->device_id = 0xFFFF; /* ATOMS Software Renderer */
    gpu->total_vram_bytes = 128 * 1024 * 1024ULL;
    gpu->aperture_size_bytes = 128 * 1024 * 1024ULL;
    gpu->is_active = true;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_shutdown(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->is_active = false;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_allocate_vram(bghal_gpu_device_t* gpu, uint64_t size_bytes, uint64_t* out_phys_addr) {
    (void)gpu;
    if (!out_phys_addr) return BVMM_ERR_INVALID_ARGUMENT;
    *out_phys_addr = 0xC0000000ULL;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_free_vram(bghal_gpu_device_t* gpu, uint64_t phys_addr, uint64_t size_bytes) {
    (void)gpu; (void)phys_addr; (void)size_bytes;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_dma_copy(bghal_gpu_device_t* gpu, const bghal_dma_req_t* dma_req) {
    (void)gpu; (void)dma_req;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_fence_signal(bghal_gpu_device_t* gpu, uint64_t timeline_val) {
    (void)gpu; (void)timeline_val;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_fence_wait(bghal_gpu_device_t* gpu, uint64_t timeline_val, uint32_t timeout_ms) {
    (void)gpu; (void)timeline_val; (void)timeout_ms;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_page_table_update(bghal_gpu_device_t* gpu, const bghal_page_table_t* pt) {
    (void)gpu; (void)pt;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_cache_flush(bghal_gpu_device_t* gpu) {
    (void)gpu;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_tlb_invalidate(bghal_gpu_device_t* gpu) {
    (void)gpu;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_handle_interrupt(bghal_gpu_device_t* gpu, uint32_t irq_vec) {
    (void)gpu; (void)irq_vec;
    return BVMM_SUCCESS;
}

static bvmm_result_t swrender_query_capabilities(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps) {
    if (!gpu || !out_caps) return BVMM_ERR_INVALID_ARGUMENT;
    out_caps->max_texture_dim = 8192;
    out_caps->max_single_alloc_bytes = 128 * 1024 * 1024ULL;
    out_caps->max_dma_transfer_bytes = 32 * 1024 * 1024ULL;
    out_caps->has_hardware_timeline_fences = false;
    out_caps->has_bar_resize = false;
    out_caps->has_coherent_memory = true;
    return BVMM_SUCCESS;
}

static const bghal_backend_vtbl_t g_swrender_vtbl = {
    .init = swrender_init,
    .shutdown = swrender_shutdown,
    .allocate_vram = swrender_allocate_vram,
    .free_vram = swrender_free_vram,
    .dma_copy = swrender_dma_copy,
    .fence_signal = swrender_fence_signal,
    .fence_wait = swrender_fence_wait,
    .page_table_update = swrender_page_table_update,
    .cache_flush = swrender_cache_flush,
    .tlb_invalidate = swrender_tlb_invalidate,
    .handle_interrupt = swrender_handle_interrupt,
    .query_capabilities = swrender_query_capabilities
};

const bghal_backend_vtbl_t* bghal_get_swrender_backend(void) {
    return &g_swrender_vtbl;
}
