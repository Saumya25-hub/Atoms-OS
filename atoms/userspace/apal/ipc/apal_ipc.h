/*
 * ATOMS Platform Adaptation Layer (APAL)
 * IPC / Mojo Message Pipe Adapter
 */

#ifndef ATOMS_APAL_IPC_H
#define ATOMS_APAL_IPC_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APAL_IPC_MAX_MSG_SIZE 4096

typedef apal_handle_t apal_ipc_handle_t;

/* Creates a bidirectional IPC message pipe pair (handle0 and handle1) */
apal_status_t apal_ipc_pipe_create(apal_ipc_handle_t *out_handle0, apal_ipc_handle_t *out_handle1);

/* Creates a named IPC channel (server end) */
apal_status_t apal_ipc_channel_create_named(const char *name, apal_ipc_handle_t *out_handle);

/* Connects to an existing named IPC channel (client end) */
apal_status_t apal_ipc_channel_connect_named(const char *name, apal_ipc_handle_t *out_handle);

/* Sends message data across the IPC handle */
apal_status_t apal_ipc_send(apal_ipc_handle_t handle, const void *data, size_t size);

/* Receives message data from the IPC handle */
apal_status_t apal_ipc_recv(apal_ipc_handle_t handle, void *out_data, size_t max_size, size_t *out_actual_size);

/* Closes an IPC handle */
apal_status_t apal_ipc_close(apal_ipc_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_IPC_H */
