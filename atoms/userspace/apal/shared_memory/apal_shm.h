/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Shared Memory Adapter (Chromium PlatformSharedMemoryRegion)
 */

#ifndef ATOMS_APAL_SHM_H
#define ATOMS_APAL_SHM_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef apal_handle_t apal_shm_handle_t;

typedef struct {
    apal_shm_handle_t handle;
    void *mapped_addr;
    size_t size;
    bool is_readonly;
} apal_shm_mapping_t;

/* Creates a named or anonymous shared memory buffer */
apal_status_t apal_shm_create(size_t size, bool readonly, apal_shm_handle_t *out_handle);

/* Creates a named shared memory buffer */
apal_status_t apal_shm_create_named(const char *name, size_t size, bool readonly, apal_shm_handle_t *out_handle);

/* Opens an existing named shared memory buffer */
apal_status_t apal_shm_open_named(const char *name, bool readonly, apal_shm_handle_t *out_handle);

/* Maps the shared memory buffer into the process virtual address space */
apal_status_t apal_shm_map(apal_shm_handle_t handle, size_t size, bool readonly, apal_shm_mapping_t *out_mapping);

/* Unmaps the shared memory buffer */
apal_status_t apal_shm_unmap(apal_shm_mapping_t *mapping);

/* Closes the shared memory handle */
apal_status_t apal_shm_close(apal_shm_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_SHM_H */
