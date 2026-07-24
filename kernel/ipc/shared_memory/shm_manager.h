/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * shm_manager.h — Shared Memory Object Lifecycle Manager
 */

#ifndef BOS_IPC_SHM_MANAGER_H
#define BOS_IPC_SHM_MANAGER_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_shm_manager_init(void);

ipc_status_t ipc_shm_create(const char* name, uint32_t size_bytes,
                              uint32_t flags, uint32_t owner_pid,
                              ipc_shm_handle_t* out_handle);
ipc_status_t ipc_shm_open_by_name(const char* name, uint32_t flags,
                                   ipc_shm_handle_t* out_handle);
ipc_status_t ipc_shm_map_to_process(ipc_shm_handle_t handle,
                                     uint32_t pid, uint32_t perm,
                                     void** out_addr);
ipc_status_t ipc_shm_unmap_from_process(ipc_shm_handle_t handle,
                                         uint32_t pid);
ipc_status_t ipc_shm_close_handle(ipc_shm_handle_t handle);
ipc_status_t ipc_shm_destroy_object(ipc_shm_handle_t handle);
ipc_shm_object_t* ipc_shm_get(ipc_shm_handle_t handle);
bool ipc_shm_is_valid(ipc_shm_handle_t handle);

#endif /* BOS_IPC_SHM_MANAGER_H */
