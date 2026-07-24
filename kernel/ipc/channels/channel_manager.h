/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * channel_manager.h — Channel Lifecycle Manager
 */

#ifndef BOS_IPC_CHANNEL_MANAGER_H
#define BOS_IPC_CHANNEL_MANAGER_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_channel_manager_init(void);

ipc_status_t ipc_channel_create(const char* name, uint32_t flags,
                                 uint32_t owner_pid,
                                 ipc_channel_handle_t* out_handle);
ipc_status_t ipc_channel_connect_by_name(const char* name,
                                          uint32_t peer_pid,
                                          ipc_channel_handle_t* out_handle);
ipc_status_t ipc_channel_close(ipc_channel_handle_t handle);
ipc_channel_t* ipc_channel_get(ipc_channel_handle_t handle);
bool ipc_channel_is_valid(ipc_channel_handle_t handle);

#endif /* BOS_IPC_CHANNEL_MANAGER_H */
