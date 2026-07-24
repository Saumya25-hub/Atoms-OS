/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_api.h — Public API Declarations
 *
 * All public-facing IPC and Shared Memory function declarations.
 * This is the ONLY header external code should include.
 */

#ifndef BOS_IPC_API_H
#define BOS_IPC_API_H

#include "kernel/ipc/include/ipc_types.h"

/* ============================================================
 * Subsystem Lifecycle
 * ============================================================ */
ipc_status_t bos_ipc_init(void);
ipc_status_t bos_ipc_shutdown(void);

/* ============================================================
 * Channel API
 * ============================================================ */
ipc_status_t bos_ipc_create_channel(const char* name, uint32_t flags,
                                     ipc_channel_handle_t* out_handle);
ipc_status_t bos_ipc_connect(const char* name,
                              ipc_channel_handle_t* out_handle);
ipc_status_t bos_ipc_send(ipc_channel_handle_t channel,
                           const void* data, uint32_t size, uint32_t flags);
ipc_status_t bos_ipc_receive(ipc_channel_handle_t channel,
                              void* buffer, uint32_t buffer_size,
                              uint32_t* out_size, uint32_t flags);
ipc_status_t bos_ipc_close(ipc_channel_handle_t channel);

/* ============================================================
 * Shared Memory API
 * ============================================================ */
ipc_status_t bos_shm_create(const char* name, uint32_t size,
                             uint32_t flags, ipc_shm_handle_t* out_handle);
ipc_status_t bos_shm_open(const char* name, uint32_t flags,
                            ipc_shm_handle_t* out_handle);
ipc_status_t bos_shm_map(ipc_shm_handle_t handle, uint32_t pid,
                           uint32_t perm, void** out_addr);
ipc_status_t bos_shm_unmap(ipc_shm_handle_t handle, uint32_t pid);
ipc_status_t bos_shm_close(ipc_shm_handle_t handle);
ipc_status_t bos_shm_destroy(ipc_shm_handle_t handle);

/* ============================================================
 * Pipe API
 * ============================================================ */
ipc_status_t bos_pipe_create(uint32_t reader_pid, uint32_t writer_pid,
                              ipc_pipe_handle_t* out_handle);
ipc_status_t bos_pipe_write(ipc_pipe_handle_t handle,
                             const void* data, uint32_t size,
                             uint32_t* out_written);
ipc_status_t bos_pipe_read(ipc_pipe_handle_t handle,
                            void* buffer, uint32_t buffer_size,
                            uint32_t* out_read);
ipc_status_t bos_pipe_close(ipc_pipe_handle_t handle);

/* ============================================================
 * Port API
 * ============================================================ */
ipc_status_t bos_port_create(const char* name, uint32_t owner_pid,
                              ipc_port_handle_t* out_handle);
ipc_status_t bos_port_bind(ipc_port_handle_t port,
                            ipc_channel_handle_t channel);
ipc_status_t bos_port_lookup(const char* name,
                              ipc_port_handle_t* out_handle);
ipc_status_t bos_port_close(ipc_port_handle_t port);

/* ============================================================
 * Router API
 * ============================================================ */
ipc_status_t bos_ipc_broadcast(ipc_channel_handle_t channel,
                                const void* data, uint32_t size);
ipc_status_t bos_ipc_subscribe(ipc_channel_handle_t channel,
                                uint32_t subscriber_pid);

/* ============================================================
 * Test Suite
 * ============================================================ */
bool ipc_run_unit_tests(void);

#endif /* BOS_IPC_API_H */
