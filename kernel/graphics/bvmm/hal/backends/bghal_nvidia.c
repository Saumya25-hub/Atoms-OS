#include "../bghal.h"

static bvmm_result_t nvidia_init(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->vendor_id = BGHAL_VENDOR_NVIDIA;
    gpu->device_id = 0x2206; /* NVIDIA GeForce RTX 3080 */
    gpu->total_vram_bytes = 10240ULL * 1024 * 1024ULL;
    gpu->aperture_size_bytes = 1024 * 1024 * 1024ULL;
    gpu->is_active = true;
    return BVMM_SUCCESS;
}

static bvmm_result_t nvidia_shutdown(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->is_active = false;
    return BVMM_SUCCESS;
}

static bvmm_result_t nvidia_query_capabilities(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps) {
    if (!gpu || !out_caps) return BVMM_ERR_INVALID_ARGUMENT;
    out_caps->max_texture_dim = 32768;
    out_caps->max_single_alloc_bytes = 4096ULL * 1024 * 1024ULL;
    out_caps->max_dma_transfer_bytes = 256 * 1024 * 1024ULL;
    out_caps->has_hardware_timeline_fences = true;
    out_caps->has_bar_resize = true;
    out_caps->has_compression = true;
    return BVMM_SUCCESS;
}

static const bghal_backend_vtbl_t g_nvidia_vtbl = {
    .init = nvidia_init,
    .shutdown = nvidia_shutdown,
    .query_capabilities = nvidia_query_capabilities
};

const bghal_backend_vtbl_t* bghal_get_nvidia_backend(void) {
    return &g_nvidia_vtbl;
}
