/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_permissions.c — Access Control & Validation Implementation
 */

#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/debug/ipc_debug.h"

void ipc_permissions_init(void) {
    ipc_debug_log(IPC_LOG_INFO, "PERMISSIONS", "Permission Validation Engine Initialized");
}

ipc_status_t ipc_perm_validate_channel_handle(ipc_channel_handle_t handle) {
    if (handle == IPC_INVALID_HANDLE) {
        ipc_debug_log(IPC_LOG_WARN, "PERMISSIONS", "Rejected invalid channel handle");
        return IPC_ERR_INVALID_HANDLE;
    }
    if (handle >= IPC_MAX_CHANNELS) {
        ipc_debug_log(IPC_LOG_WARN, "PERMISSIONS", "Channel handle out of range");
        return IPC_ERR_INVALID_HANDLE;
    }
    return IPC_SUCCESS;
}

ipc_status_t ipc_perm_validate_shm_handle(ipc_shm_handle_t handle) {
    if (handle == IPC_INVALID_HANDLE) {
        ipc_debug_log(IPC_LOG_WARN, "PERMISSIONS", "Rejected invalid SHM handle");
        return IPC_ERR_INVALID_HANDLE;
    }
    if (handle >= IPC_MAX_SHM_OBJECTS) {
        ipc_debug_log(IPC_LOG_WARN, "PERMISSIONS", "SHM handle out of range");
        return IPC_ERR_INVALID_HANDLE;
    }
    return IPC_SUCCESS;
}

ipc_status_t ipc_perm_validate_pipe_handle(ipc_pipe_handle_t handle) {
    if (handle == IPC_INVALID_HANDLE) return IPC_ERR_INVALID_HANDLE;
    if (handle >= IPC_MAX_PIPES) return IPC_ERR_INVALID_HANDLE;
    return IPC_SUCCESS;
}

ipc_status_t ipc_perm_validate_port_handle(ipc_port_handle_t handle) {
    if (handle == IPC_INVALID_HANDLE) return IPC_ERR_INVALID_HANDLE;
    if (handle >= IPC_MAX_PORTS) return IPC_ERR_INVALID_HANDLE;
    return IPC_SUCCESS;
}

ipc_status_t ipc_perm_validate_shm_access(ipc_shm_handle_t handle,
                                           uint32_t pid, uint32_t requested_perm) {
    if (handle == IPC_INVALID_HANDLE || handle >= IPC_MAX_SHM_OBJECTS) {
        return IPC_ERR_INVALID_HANDLE;
    }
    /* Permission check is further validated by shm_manager against mapping table */
    (void)pid;
    (void)requested_perm;
    return IPC_SUCCESS;
}

ipc_status_t ipc_perm_check_name(const char* name) {
    if (!name) return IPC_ERR_INVALID_NAME;
    uint32_t len = 0;
    while (name[len] != '\0') {
        len++;
        if (len > IPC_MAX_NAME_LENGTH) {
            ipc_debug_log(IPC_LOG_WARN, "PERMISSIONS", "Name exceeds maximum length");
            return IPC_ERR_INVALID_NAME;
        }
    }
    if (len == 0) return IPC_ERR_INVALID_NAME;
    return IPC_SUCCESS;
}
