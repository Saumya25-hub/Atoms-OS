/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * channel_manager.c — Channel Lifecycle Manager Implementation
 */

#include "kernel/ipc/channels/channel_manager.h"
#include "kernel/ipc/message/message_queue.h"
#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/debug/ipc_debug.h"

/* Static channel table — no heap allocation for core structures */
static ipc_channel_t g_channels[IPC_MAX_CHANNELS];
static uint32_t      g_channel_count = 0;
static uint32_t      g_next_channel_id = 1;

/* Simple inline string compare (no libc) */
static bool ipc_str_equal(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return (*a == *b);
}

static void ipc_str_copy(char* dst, const char* src, uint32_t max_len) {
    uint32_t i = 0;
    while (src[i] && i < max_len) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void ipc_channel_manager_init(void) {
    for (uint32_t i = 0; i < IPC_MAX_CHANNELS; i++) {
        g_channels[i].id        = 0;
        g_channels[i].state     = IPC_CHANNEL_STATE_FREE;
        g_channels[i].ref_count = 0;
        g_channels[i].name[0]   = '\0';
    }
    g_channel_count   = 0;
    g_next_channel_id = 1;
    ipc_debug_log(IPC_LOG_INFO, "CHANNEL_MGR", "Channel Manager Initialized (64 slots)");
}

ipc_status_t ipc_channel_create(const char* name, uint32_t flags,
                                 uint32_t owner_pid,
                                 ipc_channel_handle_t* out_handle) {
    if (!out_handle) return IPC_ERR_NULL_POINTER;
    *out_handle = IPC_INVALID_HANDLE;

    /* Validate name if named channel */
    if (flags & IPC_CHANNEL_NAMED) {
        ipc_status_t ns = ipc_perm_check_name(name);
        if (ns != IPC_SUCCESS) return ns;

        /* Check for duplicate name */
        for (uint32_t i = 0; i < IPC_MAX_CHANNELS; i++) {
            if (g_channels[i].state != IPC_CHANNEL_STATE_FREE &&
                ipc_str_equal(g_channels[i].name, name)) {
                return IPC_ERR_NAME_EXISTS;
            }
        }
    }

    /* Find free slot */
    for (uint32_t i = 0; i < IPC_MAX_CHANNELS; i++) {
        if (g_channels[i].state == IPC_CHANNEL_STATE_FREE) {
            g_channels[i].id             = g_next_channel_id++;
            g_channels[i].flags          = flags;
            g_channels[i].state          = IPC_CHANNEL_STATE_OPEN;
            g_channels[i].owner_pid      = owner_pid;
            g_channels[i].peer_pid       = 0;
            g_channels[i].ref_count      = 1;
            g_channels[i].messages_sent  = 0;
            g_channels[i].messages_received = 0;

            if ((flags & IPC_CHANNEL_NAMED) && name) {
                ipc_str_copy(g_channels[i].name, name, IPC_MAX_NAME_LENGTH);
            } else {
                g_channels[i].name[0] = '\0';
            }

            ipc_mq_clear(i);
            *out_handle = i;
            g_channel_count++;
            return IPC_SUCCESS;
        }
    }

    ipc_debug_log(IPC_LOG_ERROR, "CHANNEL_MGR", "Maximum channels reached");
    return IPC_ERR_MAX_CHANNELS;
}

ipc_status_t ipc_channel_connect_by_name(const char* name,
                                          uint32_t peer_pid,
                                          ipc_channel_handle_t* out_handle) {
    if (!name || !out_handle) return IPC_ERR_NULL_POINTER;
    *out_handle = IPC_INVALID_HANDLE;

    for (uint32_t i = 0; i < IPC_MAX_CHANNELS; i++) {
        if (g_channels[i].state == IPC_CHANNEL_STATE_OPEN &&
            ipc_str_equal(g_channels[i].name, name)) {
            g_channels[i].peer_pid  = peer_pid;
            g_channels[i].state     = IPC_CHANNEL_STATE_CONNECTED;
            g_channels[i].ref_count++;
            *out_handle = i;
            return IPC_SUCCESS;
        }
    }

    return IPC_ERR_NAME_NOT_FOUND;
}

ipc_status_t ipc_channel_close(ipc_channel_handle_t handle) {
    ipc_status_t vs = ipc_perm_validate_channel_handle(handle);
    if (vs != IPC_SUCCESS) return vs;

    ipc_channel_t* ch = &g_channels[handle];
    if (ch->state == IPC_CHANNEL_STATE_FREE) return IPC_ERR_ALREADY_DESTROYED;

    if (ch->ref_count > 0) ch->ref_count--;
    if (ch->ref_count == 0) {
        ch->state    = IPC_CHANNEL_STATE_FREE;
        ch->name[0]  = '\0';
        ch->id       = 0;
        if (g_channel_count > 0) g_channel_count--;
    } else {
        ch->state = IPC_CHANNEL_STATE_CLOSED;
    }

    return IPC_SUCCESS;
}

ipc_channel_t* ipc_channel_get(ipc_channel_handle_t handle) {
    if (handle >= IPC_MAX_CHANNELS) return (void*)0;
    if (g_channels[handle].state == IPC_CHANNEL_STATE_FREE) return (void*)0;
    return &g_channels[handle];
}

bool ipc_channel_is_valid(ipc_channel_handle_t handle) {
    if (handle >= IPC_MAX_CHANNELS) return false;
    return (g_channels[handle].state != IPC_CHANNEL_STATE_FREE);
}
