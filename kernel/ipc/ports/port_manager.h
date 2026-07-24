/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * port_manager.h — Named Port Registry
 */

#ifndef BOS_IPC_PORT_MANAGER_H
#define BOS_IPC_PORT_MANAGER_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_port_manager_init(void);

ipc_status_t ipc_port_create(const char* name, uint32_t owner_pid,
                              ipc_port_handle_t* out_handle);
ipc_status_t ipc_port_bind(ipc_port_handle_t port,
                            ipc_channel_handle_t channel);
ipc_status_t ipc_port_lookup(const char* name,
                              ipc_port_handle_t* out_handle);
ipc_status_t ipc_port_close(ipc_port_handle_t port);
bool         ipc_port_is_valid(ipc_port_handle_t handle);

#endif /* BOS_IPC_PORT_MANAGER_H */
