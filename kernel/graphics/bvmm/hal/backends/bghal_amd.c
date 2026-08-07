#include "../bghal.h"

static bvmm_result_t amd_init(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->vendor_id = BGHAL_VENDOR_AMD;
    gpu->device_id = 0x73BF; /* AMD Radeon RX 6800 XT */
    gpu->total_vram_bytes = 16384ULL * 1024 * 1024ULL;
    gpu->aperture_size_bytes = 2048 * 1024 * 1024ULL;
    gpu->is_active = true;
    return BVMM_SUCCESS;
}

static bvmm_result_t amd_shutdown(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->is_active = false;
    return BVMM_SUCCESS;
}

static bvmm_result_t amd_query_capabilities(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps) {
    if (!gpu || !out_caps) return BVMM_ERR_INVALID_ARGUMENT;
    out_caps->max_texture_dim = 16384;
    out_caps->max_single_alloc_bytes = 4096ULL * 1024 * 1024ULL;
    out_caps->max_dma_transfer_bytes = 128 * 1024 * 1024ULL;
    out_caps->has_hardware_timeline_fences = true;
    out_caps->has_bar_resize = true;
    out_caps->has_compression = true;
    return BVMM_SUCCESS;
}

static const bghal_backend_vtbl_t g_amd_vtbl = {
    .init = amd_init,
    .shutdown = amd_shutdown,
    .query_capabilities = amd_query_capabilities
};

const bghal_backend_vtbl_t* bghal_get_amd_backend(void) {
    return &g_amd_vtbl;
}
