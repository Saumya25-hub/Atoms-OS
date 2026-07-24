/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * port_manager.c — Named Port Registry Implementation
 */

#include "kernel/ipc/ports/port_manager.h"
#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/debug/ipc_debug.h"

static ipc_port_t g_ports[IPC_MAX_PORTS];
static uint32_t   g_next_port_id = 1;

static bool ipc_port_str_equal(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return (*a == *b);
}

static void ipc_port_str_copy(char* dst, const char* src, uint32_t max_len) {
    uint32_t i = 0;
    while (src[i] && i < max_len) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void ipc_port_manager_init(void) {
    for (uint32_t i = 0; i < IPC_MAX_PORTS; i++) {
        g_ports[i].id    = 0;
        g_ports[i].state = IPC_PORT_STATE_FREE;
        g_ports[i].name[0] = '\0';
        g_ports[i].ref_count = 0;
    }
    g_next_port_id = 1;
    ipc_debug_log(IPC_LOG_INFO, "PORT_MGR", "Port Manager Initialized");
}

ipc_status_t ipc_port_create(const char* name, uint32_t owner_pid,
                              ipc_port_handle_t* out_handle) {
    if (!name || !out_handle) return IPC_ERR_NULL_POINTER;
    *out_handle = IPC_INVALID_HANDLE;

    ipc_status_t ns = ipc_perm_check_name(name);
    if (ns != IPC_SUCCESS) return ns;

    /* Check for duplicate name */
    for (uint32_t i = 0; i < IPC_MAX_PORTS; i++) {
        if (g_ports[i].state != IPC_PORT_STATE_FREE &&
            ipc_port_str_equal(g_ports[i].name, name)) {
            return IPC_ERR_NAME_EXISTS;
        }
    }

    for (uint32_t i = 0; i < IPC_MAX_PORTS; i++) {
        if (g_ports[i].state == IPC_PORT_STATE_FREE) {
            g_ports[i].id         = g_next_port_id++;
            g_ports[i].state      = IPC_PORT_STATE_BOUND;
            g_ports[i].owner_pid  = owner_pid;
            g_ports[i].bound_channel = IPC_INVALID_HANDLE;
            g_ports[i].ref_count  = 1;
            ipc_port_str_copy(g_ports[i].name, name, IPC_MAX_NAME_LENGTH);
            *out_handle = i;
            return IPC_SUCCESS;
        }
    }

    return IPC_ERR_MAX_PORTS;
}

ipc_status_t ipc_port_bind(ipc_port_handle_t port,
                            ipc_channel_handle_t channel) {
    ipc_status_t vs = ipc_perm_validate_port_handle(port);
    if (vs != IPC_SUCCESS) return vs;

    if (g_ports[port].state == IPC_PORT_STATE_FREE) return IPC_ERR_INVALID_HANDLE;
    g_ports[port].bound_channel = channel;
    return IPC_SUCCESS;
}

ipc_status_t ipc_port_lookup(const char* name,
                              ipc_port_handle_t* out_handle) {
    if (!name || !out_handle) return IPC_ERR_NULL_POINTER;
    *out_handle = IPC_INVALID_HANDLE;

    for (uint32_t i = 0; i < IPC_MAX_PORTS; i++) {
        if (g_ports[i].state != IPC_PORT_STATE_FREE &&
            ipc_port_str_equal(g_ports[i].name, name)) {
            *out_handle = i;
            return IPC_SUCCESS;
        }
    }

    return IPC_ERR_NAME_NOT_FOUND;
}

ipc_status_t ipc_port_close(ipc_port_handle_t port) {
    ipc_status_t vs = ipc_perm_validate_port_handle(port);
    if (vs != IPC_SUCCESS) return vs;

    if (g_ports[port].state == IPC_PORT_STATE_FREE) return IPC_ERR_ALREADY_DESTROYED;

    g_ports[port].state   = IPC_PORT_STATE_FREE;
    g_ports[port].name[0] = '\0';
    g_ports[port].id      = 0;
    g_ports[port].ref_count = 0;
    return IPC_SUCCESS;
}

bool ipc_port_is_valid(ipc_port_handle_t handle) {
    if (handle >= IPC_MAX_PORTS) return false;
    return (g_ports[handle].state != IPC_PORT_STATE_FREE);
}
