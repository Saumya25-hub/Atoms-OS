/*
 * ATOMS Platform Adaptation Layer (APAL)
 * IPC / Mojo Message Pipe Implementation
 * Backed by ATOMS OS in-kernel IPC channel engine (SYS_IPC_CALL)
 */

#include "apal_ipc.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

static volatile uint32_t g_pipe_seq = 1;

apal_status_t apal_ipc_pipe_create(apal_ipc_handle_t *out_handle0, apal_ipc_handle_t *out_handle1) {
    if (!out_handle0 || !out_handle1) return APAL_ERR_INVALID_PARAM;

    char chan_name[32];
    uint32_t seq = __atomic_fetch_add(&g_pipe_seq, 1, __ATOMIC_SEQ_CST);
    /* Generate unique anonymous channel name */
    chan_name[0] = 'm'; chan_name[1] = 'o'; chan_name[2] = 'j'; chan_name[3] = 'o'; chan_name[4] = '_';
    int p = 5;
    uint32_t tmp = seq;
    char rev[12]; int r = 0;
    if (tmp == 0) rev[r++] = '0';
    while (tmp > 0) { rev[r++] = '0' + (tmp % 10); tmp /= 10; }
    while (r > 0) chan_name[p++] = rev[--r];
    chan_name[p] = '\0';

    uint32_t h0 = 0, h1 = 0;
    int64_t res = atoms_sys_ipc_call(ATOMS_IPC_OP_CREATE, (uint64_t)chan_name, 0, (uint64_t)&h0);
    if (res != 0) return APAL_ERR_NO_MEMORY;

    res = atoms_sys_ipc_call(ATOMS_IPC_OP_CONNECT, (uint64_t)chan_name, (uint64_t)&h1, 0);
    if (res != 0) {
        atoms_sys_ipc_call(ATOMS_IPC_OP_CLOSE, h0, 0, 0);
        return APAL_ERR_NO_MEMORY;
    }

    *out_handle0 = (apal_ipc_handle_t)h0;
    *out_handle1 = (apal_ipc_handle_t)h1;
    return APAL_OK;
}

apal_status_t apal_ipc_channel_create_named(const char *name, apal_ipc_handle_t *out_handle) {
    if (!name || !out_handle) return APAL_ERR_INVALID_PARAM;
    uint32_t h = 0;
    int64_t res = atoms_sys_ipc_call(ATOMS_IPC_OP_CREATE, (uint64_t)name, 0, (uint64_t)&h);
    if (res == 0) {
        *out_handle = (apal_ipc_handle_t)h;
        return APAL_OK;
    }
    return APAL_ERR_BUSY;
}

apal_status_t apal_ipc_channel_connect_named(const char *name, apal_ipc_handle_t *out_handle) {
    if (!name || !out_handle) return APAL_ERR_INVALID_PARAM;
    uint32_t h = 0;
    int64_t res = atoms_sys_ipc_call(ATOMS_IPC_OP_CONNECT, (uint64_t)name, (uint64_t)&h, 0);
    if (res == 0) {
        *out_handle = (apal_ipc_handle_t)h;
        return APAL_OK;
    }
    return APAL_ERR_NOT_FOUND;
}

apal_status_t apal_ipc_send(apal_ipc_handle_t handle, const void *data, size_t size) {
    if (handle == 0 || !data || size == 0 || size > APAL_IPC_MAX_MSG_SIZE) return APAL_ERR_INVALID_PARAM;
    int64_t res = atoms_sys_ipc_call(ATOMS_IPC_OP_SEND, (uint64_t)handle, (uint64_t)data, (uint64_t)size);
    if (res == 0) {
        return APAL_OK;
    }
    return APAL_ERR_IO;
}

apal_status_t apal_ipc_recv(apal_ipc_handle_t handle, void *out_data, size_t max_size, size_t *out_actual_size) {
    if (handle == 0 || !out_data || max_size == 0) return APAL_ERR_INVALID_PARAM;
    int64_t res = atoms_sys_ipc_call(ATOMS_IPC_OP_RECV, (uint64_t)handle, (uint64_t)out_data, (uint64_t)max_size);
    if (res >= 0) {
        if (out_actual_size) {
            *out_actual_size = (size_t)res;
        }
        return APAL_OK;
    }
    return APAL_ERR_IO;
}

apal_status_t apal_ipc_close(apal_ipc_handle_t handle) {
    if (handle == 0) return APAL_ERR_INVALID_PARAM;
    int64_t res = atoms_sys_ipc_call(ATOMS_IPC_OP_CLOSE, (uint64_t)handle, 0, 0);
    return (res == 0) ? APAL_OK : APAL_ERR_INVALID_PARAM;
}
