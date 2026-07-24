/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * pipe_engine.h — Unidirectional Byte Stream Pipes
 */

#ifndef BOS_IPC_PIPE_ENGINE_H
#define BOS_IPC_PIPE_ENGINE_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_pipe_engine_init(void);

ipc_status_t ipc_pipe_create(uint32_t reader_pid, uint32_t writer_pid,
                              ipc_pipe_handle_t* out_handle);
ipc_status_t ipc_pipe_write(ipc_pipe_handle_t handle,
                             const void* data, uint32_t size,
                             uint32_t* out_written);
ipc_status_t ipc_pipe_read(ipc_pipe_handle_t handle,
                            void* buffer, uint32_t buffer_size,
                            uint32_t* out_read);
ipc_status_t ipc_pipe_close(ipc_pipe_handle_t handle);
bool         ipc_pipe_is_valid(ipc_pipe_handle_t handle);

#endif /* BOS_IPC_PIPE_ENGINE_H */
