/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * message_queue.h — Per-Channel Message Queue Engine
 */

#ifndef BOS_IPC_MESSAGE_QUEUE_H
#define BOS_IPC_MESSAGE_QUEUE_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_message_queue_init(void);
void ipc_mq_clear(ipc_channel_handle_t channel);

ipc_status_t ipc_mq_enqueue(ipc_channel_handle_t channel,
                             const ipc_message_t* msg);
ipc_status_t ipc_mq_dequeue(ipc_channel_handle_t channel,
                             ipc_message_t* out_msg, uint32_t flags);
uint32_t     ipc_mq_pending_count(ipc_channel_handle_t channel);
bool         ipc_mq_is_full(ipc_channel_handle_t channel);
bool         ipc_mq_is_empty(ipc_channel_handle_t channel);

#endif /* BOS_IPC_MESSAGE_QUEUE_H */
