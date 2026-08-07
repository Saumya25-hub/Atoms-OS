#include "bvmm_pools.h"

/**
 * @file bvmm_pool_policy.c
 * @brief Pool Placement, Lifetime & Flag Validation Engine
 */

bool bvmm_pool_validate_lifetime(bvmm_lifetime_t lifetime) {
    switch (lifetime) {
        case BVMM_LIFETIME_PERSISTENT:
        case BVMM_LIFETIME_TEMPORARY:
        case BVMM_LIFETIME_FRAME:
        case BVMM_LIFETIME_WINDOW:
        case BVMM_LIFETIME_PROCESS:
        case BVMM_LIFETIME_DEVICE:
        case BVMM_LIFETIME_SHARED:
            return true;
        default:
            return false;
    }
}

bool bvmm_pool_validate_flags(uint32_t flags) {
    /* Check for conflicting flag combinations */
    if ((flags & BVMM_POOL_FLAG_GPU_ONLY) && (flags & BVMM_POOL_FLAG_CPU_VISIBLE)) {
        return false; /* Cannot be GPU_ONLY and CPU_VISIBLE simultaneously */
    }
    if ((flags & BVMM_POOL_FLAG_UPLOAD) && (flags & BVMM_POOL_FLAG_READBACK)) {
        return false; /* Cannot be UPLOAD and READBACK simultaneously */
    }
    return true;
}

uint32_t bvmm_pool_compute_pressure(size_t allocated_bytes, size_t capacity_bytes) {
    if (capacity_bytes == 0) return 0;
    uint64_t pressure = (allocated_bytes * 100ULL) / capacity_bytes;
    return (pressure > 100) ? 100 : (uint32_t)pressure;
}
