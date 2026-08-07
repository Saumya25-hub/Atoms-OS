#include "../include/bvmm_api.h"
#include "../pools/bvmm_pools.h"
#include "../handles/bvmm_handle_table.h"
#include "../diagnostics/bvmm_stats.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

bvmm_result_t bvmm_allocate(const bvmm_alloc_info_t* info, bvmm_handle_t* out_handle) {
    if (!info || !out_handle) return BVMM_ERR_INVALID_ARGUMENT;
    if (!bvmm_is_initialized()) return BVMM_ERR_NOT_INITIALIZED;
    if (!bvmm_validate_alloc_info(info)) {
        bvmm_stats_record_failure();
        return BVMM_ERR_INVALID_ARGUMENT;
    }

    /* Route allocation through Production Memory Pool Engine (PMPE) */
    return bvmm_pool_allocate(info->pool_type, info, out_handle);
}

bvmm_result_t bvmm_free(bvmm_handle_t handle) {
    if (handle == BVMM_INVALID_HANDLE) return BVMM_ERR_INVALID_HANDLE;
    if (!bvmm_is_initialized()) return BVMM_ERR_NOT_INITIALIZED;

    /* Delegate release to Production Memory Pool Engine */
    return bvmm_pool_free(handle);
}

bvmm_result_t bvmm_map(bvmm_handle_t handle, void** out_cpu_ptr) {
    if (handle == BVMM_INVALID_HANDLE || !out_cpu_ptr) return BVMM_ERR_INVALID_ARGUMENT;
    if (!bvmm_is_initialized()) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_allocation_t* alloc = NULL;
    bvmm_result_t res = bvmm_handle_table_lookup(handle, &alloc);
    if (res != BVMM_SUCCESS || !alloc) return res;

    *out_cpu_ptr = alloc->cpu_virtual_addr;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_unmap(bvmm_handle_t handle) {
    if (handle == BVMM_INVALID_HANDLE) return BVMM_ERR_INVALID_HANDLE;
    if (!bvmm_is_initialized()) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_allocation_t* alloc = NULL;
    bvmm_result_t res = bvmm_handle_table_lookup(handle, &alloc);
    if (res != BVMM_SUCCESS || !alloc) return res;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_import(uint32_t shared_id, bvmm_handle_t* out_handle) {
    if (shared_id == 0 || !out_handle) return BVMM_ERR_INVALID_ARGUMENT;
    if (!bvmm_is_initialized()) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_handle_t handle = (bvmm_handle_t)shared_id;
    bvmm_allocation_t* alloc = NULL;
    bvmm_result_t res = bvmm_handle_table_lookup(handle, &alloc);
    if (res != BVMM_SUCCESS || !alloc) return res;

    alloc->cpu_refcount++;
    *out_handle = handle;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_export(bvmm_handle_t handle, uint32_t* out_shared_id) {
    if (handle == BVMM_INVALID_HANDLE || !out_shared_id) return BVMM_ERR_INVALID_ARGUMENT;
    if (!bvmm_is_initialized()) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_allocation_t* alloc = NULL;
    bvmm_result_t res = bvmm_handle_table_lookup(handle, &alloc);
    if (res != BVMM_SUCCESS || !alloc) return res;

    *out_shared_id = (uint32_t)handle;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_get_info(bvmm_handle_t handle, bvmm_alloc_info_t* out_info) {
    if (handle == BVMM_INVALID_HANDLE || !out_info) return BVMM_ERR_INVALID_ARGUMENT;
    if (!bvmm_is_initialized()) return BVMM_ERR_NOT_INITIALIZED;

    bvmm_allocation_t* alloc = NULL;
    bvmm_result_t res = bvmm_handle_table_lookup(handle, &alloc);
    if (res != BVMM_SUCCESS || !alloc) return res;

    memset(out_info, 0, sizeof(bvmm_alloc_info_t));
    out_info->size_bytes       = alloc->size_bytes;
    out_info->alignment_bytes  = alloc->alignment_bytes;
    out_info->preferred_domain = alloc->current_domain;
    out_info->pool_type        = alloc->pool_type;
    out_info->alloc_flags      = alloc->alloc_flags;
    out_info->owner_pid        = alloc->owner_pid;
    out_info->width            = alloc->width;
    out_info->height           = alloc->height;
    out_info->stride_bytes     = alloc->stride_bytes;
    out_info->format_fourcc    = alloc->format_fourcc;

    return BVMM_SUCCESS;
}
