#ifndef _BOS_BVMM_API_H_
#define _BOS_BVMM_API_H_

#include "bvmm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_api.h
 * @brief BOS VRAM Memory Manager (BVMM) Public API Declarations
 * 
 * Central API entrypoints for initializing, shutting down, allocating,
 * freeing, mapping, unmapping, importing, exporting, and querying BVMM.
 */

/**
 * @brief Initialize the BVMM subsystem and core memory structures.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_init(void);

/**
 * @brief Shutdown BVMM subsystem, releasing all remaining resources.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_shutdown(void);

/**
 * @brief Check if the BVMM subsystem is currently initialized.
 * @return true if initialized, false otherwise.
 */
bool bvmm_is_initialized(void);

/**
 * @brief Allocate a video memory object.
 * @param info Requested allocation attributes (size, domain, pool, alignment).
 * @param out_handle Pointer to receive the created handle.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_allocate(const bvmm_alloc_info_t* info, bvmm_handle_t* out_handle);

/**
 * @brief Free a video memory object allocated by BVMM.
 * @param handle Handle of allocation to free.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_free(bvmm_handle_t handle);

/**
 * @brief Map a BVMM allocation into CPU virtual address space.
 * @param handle Handle of allocation to map.
 * @param out_cpu_ptr Pointer to receive the mapped CPU virtual address.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_map(bvmm_handle_t handle, void** out_cpu_ptr);

/**
 * @brief Unmap a previously mapped BVMM allocation from CPU address space.
 * @param handle Handle of allocation to unmap.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_unmap(bvmm_handle_t handle);

/**
 * @brief Import a shared cross-process allocation handle.
 * @param shared_id Globally exported handle identifier.
 * @param out_handle Pointer to receive process-local handle.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_import(uint32_t shared_id, bvmm_handle_t* out_handle);

/**
 * @brief Export an allocation handle for cross-process sharing.
 * @param handle Local allocation handle.
 * @param out_shared_id Pointer to receive globally exportable identifier.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_export(bvmm_handle_t handle, uint32_t* out_shared_id);

/**
 * @brief Query detailed descriptor information for a given allocation handle.
 * @param handle Allocation handle to query.
 * @param out_info Pointer to receive allocation info descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_get_info(bvmm_handle_t handle, bvmm_alloc_info_t* out_info);

/**
 * @brief Retrieve current BVMM telemetry and forensic statistics.
 * @param out_stats Pointer to receive statistics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_get_stats(bvmm_stats_t* out_stats);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_API_H_ */
