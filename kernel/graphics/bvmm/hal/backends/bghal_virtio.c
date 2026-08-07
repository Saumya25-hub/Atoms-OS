#include "../bghal.h"

static bvmm_result_t virtio_init(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->vendor_id = BGHAL_VENDOR_VIRTIO;
    gpu->device_id = 0x1050; /* VirtIO GPU Device */
    gpu->total_vram_bytes = 256 * 1024 * 1024ULL;
    gpu->aperture_size_bytes = 256 * 1024 * 1024ULL;
    gpu->is_active = true;
    return BVMM_SUCCESS;
}

static bvmm_result_t virtio_shutdown(bghal_gpu_device_t* gpu) {
    if (!gpu) return BVMM_ERR_INVALID_ARGUMENT;
    gpu->is_active = false;
    return BVMM_SUCCESS;
}

static bvmm_result_t virtio_query_capabilities(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps) {
    if (!gpu || !out_caps) return BVMM_ERR_INVALID_ARGUMENT;
    out_caps->max_texture_dim = 8192;
    out_caps->max_single_alloc_bytes = 128 * 1024 * 1024ULL;
    out_caps->max_dma_transfer_bytes = 32 * 1024 * 1024ULL;
    out_caps->has_hardware_timeline_fences = false;
    return BVMM_SUCCESS;
}

static const bghal_backend_vtbl_t g_virtio_vtbl = {
    .init = virtio_init,
    .shutdown = virtio_shutdown,
    .query_capabilities = virtio_query_capabilities
};

const bghal_backend_vtbl_t* bghal_get_virtio_backend(void) {
    return &g_virtio_vtbl;
}
