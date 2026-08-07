#include "../bghal.h"

static bvmm_result_t intel_init(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->vendor_id = BGHAL_VENDOR_INTEL;
    gpu->device_id = 0x9A49; /* Intel Iris Xe Graphics */
    gpu->total_vram_bytes = 2048 * 1024 * 1024ULL;
    gpu->aperture_size_bytes = 512 * 1024 * 1024ULL;
    gpu->is_active = true;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_shutdown(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->is_active = false;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_allocate_vram(bghal_gpu_device_t* gpu, uint64_t size_bytes, uint64_t* out_phys_addr) {
    (void)gpu;
    if (!out_phys_addr) return BVMM_ERR_INVALID_ARGUMENT;
    *out_phys_addr = 0xD0000000ULL;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_free_vram(bghal_gpu_device_t* gpu, uint64_t phys_addr, uint64_t size_bytes) {
    (void)gpu; (void)phys_addr; (void)size_bytes;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_dma_copy(bghal_gpu_device_t* gpu, const bghal_dma_req_t* dma_req) {
    (void)gpu; (void)dma_req;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_fence_signal(bghal_gpu_device_t* gpu, uint64_t timeline_val) {
    (void)gpu; (void)timeline_val;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_fence_wait(bghal_gpu_device_t* gpu, uint64_t timeline_val, uint32_t timeout_ms) {
    (void)gpu; (void)timeline_val; (void)timeout_ms;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_page_table_update(bghal_gpu_device_t* gpu, const bghal_page_table_t* pt) {
    (void)gpu; (void)pt;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_cache_flush(bghal_gpu_device_t* gpu) {
    (void)gpu;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_tlb_invalidate(bghal_gpu_device_t* gpu) {
    (void)gpu;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_handle_interrupt(bghal_gpu_device_t* gpu, uint32_t irq_vec) {
    (void)gpu; (void)irq_vec;
    return BVMM_SUCCESS;
}

static bvmm_result_t intel_query_capabilities(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps) {
    if (!gpu || !out_caps) return BVMM_ERR_INVALID_ARGUMENT;
    out_caps->max_texture_dim = 16384;
    out_caps->max_single_alloc_bytes = 1024 * 1024 * 1024ULL;
    out_caps->max_dma_transfer_bytes = 64 * 1024 * 1024ULL;
    out_caps->has_hardware_timeline_fences = true;
    out_caps->has_bar_resize = true;
    out_caps->has_coherent_memory = true;
    return BVMM_SUCCESS;
}

static const bghal_backend_vtbl_t g_intel_vtbl = {
    .init = intel_init,
    .shutdown = intel_shutdown,
    .allocate_vram = intel_allocate_vram,
    .free_vram = intel_free_vram,
    .dma_copy = intel_dma_copy,
    .fence_signal = intel_fence_signal,
    .fence_wait = intel_fence_wait,
    .page_table_update = intel_page_table_update,
    .cache_flush = intel_cache_flush,
    .tlb_invalidate = intel_tlb_invalidate,
    .handle_interrupt = intel_handle_interrupt,
    .query_capabilities = intel_query_capabilities
};

const bghal_backend_vtbl_t* bghal_get_intel_backend(void) {
    return &g_intel_vtbl;
}
