/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_permissions.h — Access Control & Validation
 */

#ifndef BOS_IPC_PERMISSIONS_H
#define BOS_IPC_PERMISSIONS_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_permissions_init(void);

ipc_status_t ipc_perm_validate_channel_handle(ipc_channel_handle_t handle);
ipc_status_t ipc_perm_validate_shm_handle(ipc_shm_handle_t handle);
ipc_status_t ipc_perm_validate_pipe_handle(ipc_pipe_handle_t handle);
ipc_status_t ipc_perm_validate_port_handle(ipc_port_handle_t handle);
ipc_status_t ipc_perm_validate_shm_access(ipc_shm_handle_t handle,
                                           uint32_t pid, uint32_t requested_perm);
ipc_status_t ipc_perm_check_name(const char* name);

#endif /* BOS_IPC_PERMISSIONS_H */
