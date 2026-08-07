#ifndef _BOS_BVMM_HANDLE_TABLE_H_
#define _BOS_BVMM_HANDLE_TABLE_H_

#include "../include/bvmm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_handle_table.h
 * @brief Process-Isolated Handle Table Manager Header
 * 
 * Maps opaque 64-bit handles (bvmm_handle_t) to internal kernel allocation
 * descriptors with 16-bit Generation ID double-free protection.
 */

#define BVMM_HANDLE_TABLE_MAX_ENTRIES   1024

typedef struct {
    bvmm_allocation_t*  allocation;
    uint16_t            generation_id;
    uint32_t            owner_pid;
    bool                in_use;
} bvmm_handle_entry_t;

typedef struct {
    bvmm_handle_entry_t entries[BVMM_HANDLE_TABLE_MAX_ENTRIES];
    uint32_t            active_count;
    uint16_t            next_generation;
} bvmm_handle_table_t;

/**
 * @brief Initialize the global BVMM Handle Table Manager.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_handle_table_init(void);

/**
 * @brief Shutdown the Handle Table Manager and release all open handles.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_handle_table_shutdown(void);

/**
 * @brief Register a new allocation into the handle table and return a 64-bit handle.
 * @param alloc Pointer to allocation descriptor.
 * @param out_handle Pointer to receive packed 64-bit handle.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_handle_table_insert(bvmm_allocation_t* alloc, bvmm_handle_t* out_handle);

/**
 * @brief Lookup an allocation descriptor using a 64-bit handle with generation ID validation.
 * @param handle Opaque 64-bit handle.
 * @param out_alloc Pointer to receive allocation descriptor pointer.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_handle_table_lookup(bvmm_handle_t handle, bvmm_allocation_t** out_alloc);

/**
 * @brief Remove and free a handle entry from the handle table.
 * @param handle Opaque 64-bit handle to release.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bvmm_handle_table_remove(bvmm_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_HANDLE_TABLE_H_ */
