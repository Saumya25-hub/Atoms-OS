/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_router.h — Message Routing & Dispatch Engine
 */

#ifndef BOS_IPC_ROUTER_H
#define BOS_IPC_ROUTER_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_router_init(void);

ipc_status_t ipc_router_subscribe(ipc_channel_handle_t channel,
                                   uint32_t subscriber_pid);
ipc_status_t ipc_router_broadcast(ipc_channel_handle_t channel,
                                   const ipc_message_t* msg);
ipc_status_t ipc_router_unsubscribe(ipc_channel_handle_t channel,
                                     uint32_t subscriber_pid);

#endif /* BOS_IPC_ROUTER_H */
