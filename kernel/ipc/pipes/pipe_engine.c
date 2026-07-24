/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * pipe_engine.c — Unidirectional Byte Stream Pipes Implementation
 */

#include "kernel/ipc/pipes/pipe_engine.h"
#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/debug/ipc_debug.h"

static ipc_pipe_t g_pipes[IPC_MAX_PIPES];
static uint32_t   g_next_pipe_id = 1;

void ipc_pipe_engine_init(void) {
    for (uint32_t i = 0; i < IPC_MAX_PIPES; i++) {
        g_pipes[i].id    = 0;
        g_pipes[i].state = IPC_PIPE_STATE_FREE;
        g_pipes[i].head  = 0;
        g_pipes[i].tail  = 0;
        g_pipes[i].count = 0;
        g_pipes[i].bytes_transferred = 0;
    }
    g_next_pipe_id = 1;
    ipc_debug_log(IPC_LOG_INFO, "PIPE_ENGINE", "Pipe Engine Initialized");
}

ipc_status_t ipc_pipe_create(uint32_t reader_pid, uint32_t writer_pid,
                              ipc_pipe_handle_t* out_handle) {
    if (!out_handle) return IPC_ERR_NULL_POINTER;
    *out_handle = IPC_INVALID_HANDLE;

    for (uint32_t i = 0; i < IPC_MAX_PIPES; i++) {
        if (g_pipes[i].state == IPC_PIPE_STATE_FREE) {
            g_pipes[i].id         = g_next_pipe_id++;
            g_pipes[i].state      = IPC_PIPE_STATE_OPEN;
            g_pipes[i].reader_pid = reader_pid;
            g_pipes[i].writer_pid = writer_pid;
            g_pipes[i].head       = 0;
            g_pipes[i].tail       = 0;
            g_pipes[i].count      = 0;
            g_pipes[i].bytes_transferred = 0;
            *out_handle = i;
            return IPC_SUCCESS;
        }
    }

    return IPC_ERR_MAX_PIPES;
}

ipc_status_t ipc_pipe_write(ipc_pipe_handle_t handle,
                             const void* data, uint32_t size,
                             uint32_t* out_written) {
    ipc_status_t vs = ipc_perm_validate_pipe_handle(handle);
    if (vs != IPC_SUCCESS) return vs;
    if (!data || !out_written) return IPC_ERR_NULL_POINTER;

    ipc_pipe_t* p = &g_pipes[handle];
    if (p->state != IPC_PIPE_STATE_OPEN) return IPC_ERR_PIPE_CLOSED;

    const uint8_t* src = (const uint8_t*)data;
    uint32_t written = 0;

    while (written < size && p->count < IPC_PIPE_BUFFER_SIZE) {
        p->buffer[p->tail] = src[written];
        p->tail = (p->tail + 1) % IPC_PIPE_BUFFER_SIZE;
        p->count++;
        written++;
    }

    p->bytes_transferred += written;
    *out_written = written;
    return IPC_SUCCESS;
}

ipc_status_t ipc_pipe_read(ipc_pipe_handle_t handle,
                            void* buffer, uint32_t buffer_size,
                            uint32_t* out_read) {
    ipc_status_t vs = ipc_perm_validate_pipe_handle(handle);
    if (vs != IPC_SUCCESS) return vs;
    if (!buffer || !out_read) return IPC_ERR_NULL_POINTER;

    ipc_pipe_t* p = &g_pipes[handle];
    if (p->state != IPC_PIPE_STATE_OPEN) return IPC_ERR_PIPE_CLOSED;

    uint8_t* dst = (uint8_t*)buffer;
    uint32_t read_count = 0;

    while (read_count < buffer_size && p->count > 0) {
        dst[read_count] = p->buffer[p->head];
        p->head = (p->head + 1) % IPC_PIPE_BUFFER_SIZE;
        p->count--;
        read_count++;
    }

    *out_read = read_count;
    return IPC_SUCCESS;
}

ipc_status_t ipc_pipe_close(ipc_pipe_handle_t handle) {
    ipc_status_t vs = ipc_perm_validate_pipe_handle(handle);
    if (vs != IPC_SUCCESS) return vs;

    ipc_pipe_t* p = &g_pipes[handle];
    if (p->state == IPC_PIPE_STATE_FREE) return IPC_ERR_ALREADY_DESTROYED;

    p->state = IPC_PIPE_STATE_FREE;
    p->id    = 0;
    p->head  = 0;
    p->tail  = 0;
    p->count = 0;
    return IPC_SUCCESS;
}

bool ipc_pipe_is_valid(ipc_pipe_handle_t handle) {
    if (handle >= IPC_MAX_PIPES) return false;
    return (g_pipes[handle].state == IPC_PIPE_STATE_OPEN);
}
