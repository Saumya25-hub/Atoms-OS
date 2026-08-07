#include "bpoe.h"

bvmm_result_t bpoe_fastpath_texture_lookup(btfe_texture_id_t texture_id, uint64_t* out_phys_addr) {
    if (texture_id == BTFE_INVALID_TEXTURE_ID || !out_phys_addr) return BVMM_ERR_INVALID_ARGUMENT;

    btfe_texture_desc_t* desc = NULL;
    bvmm_result_t res = btfe_texture_lookup(texture_id, &desc);
    if (res != BVMM_SUCCESS || !desc) return res;

    *out_phys_addr = 0x10000000ULL + (uint64_t)texture_id * 0x1000ULL;
    return BVMM_SUCCESS;
}

bvmm_result_t bpoe_fastpath_fence_lookup(bfse_fence_id_t fence_id, bool* out_is_signaled) {
    if (fence_id == BFSE_INVALID_FENCE_ID || !out_is_signaled) return BVMM_ERR_INVALID_ARGUMENT;

    bvmm_result_t res = bfse_wait(fence_id, 0);
    *out_is_signaled = (res == BVMM_SUCCESS);
    return BVMM_SUCCESS;
}
