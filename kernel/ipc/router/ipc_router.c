/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_router.c — Message Routing & Dispatch Implementation
 */

#include "kernel/ipc/router/ipc_router.h"
#include "kernel/ipc/channels/channel_manager.h"
#include "kernel/ipc/message/message_queue.h"
#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/debug/ipc_debug.h"

static ipc_route_t g_routes[IPC_MAX_ROUTER_ROUTES];

void ipc_router_init(void) {
    for (uint32_t i = 0; i < IPC_MAX_ROUTER_ROUTES; i++) {
        g_routes[i].active           = false;
        g_routes[i].channel_id       = 0;
        g_routes[i].subscriber_count = 0;
    }
    ipc_debug_log(IPC_LOG_INFO, "ROUTER", "IPC Router & Dispatch Engine Initialized");
}

/* Find or create route for a channel */
static ipc_route_t* router_find_or_create(ipc_channel_handle_t channel) {
    /* Find existing route */
    for (uint32_t i = 0; i < IPC_MAX_ROUTER_ROUTES; i++) {
        if (g_routes[i].active && g_routes[i].channel_id == channel) {
            return &g_routes[i];
        }
    }
    /* Create new route */
    for (uint32_t i = 0; i < IPC_MAX_ROUTER_ROUTES; i++) {
        if (!g_routes[i].active) {
            g_routes[i].active           = true;
            g_routes[i].channel_id       = channel;
            g_routes[i].subscriber_count = 0;
            return &g_routes[i];
        }
    }
    return (void*)0;
}

ipc_status_t ipc_router_subscribe(ipc_channel_handle_t channel,
                                   uint32_t subscriber_pid) {
    ipc_status_t vs = ipc_perm_validate_channel_handle(channel);
    if (vs != IPC_SUCCESS) return vs;

    ipc_route_t* route = router_find_or_create(channel);
    if (!route) return IPC_ERR_MAX_CHANNELS;

    /* Check for duplicate subscriber */
    for (uint32_t i = 0; i < route->subscriber_count; i++) {
        if (route->subscriber_pids[i] == subscriber_pid) {
            return IPC_SUCCESS; /* Already subscribed */
        }
    }

    if (route->subscriber_count >= IPC_MAX_SUBSCRIBERS) {
        return IPC_ERR_MAX_CHANNELS;
    }

    route->subscriber_pids[route->subscriber_count++] = subscriber_pid;
    return IPC_SUCCESS;
}

ipc_status_t ipc_router_broadcast(ipc_channel_handle_t channel,
                                   const ipc_message_t* msg) {
    ipc_status_t vs = ipc_perm_validate_channel_handle(channel);
    if (vs != IPC_SUCCESS) return vs;
    if (!msg) return IPC_ERR_NULL_POINTER;
    if (!ipc_channel_is_valid(channel)) return IPC_ERR_INVALID_HANDLE;

    /* Enqueue message into the broadcast channel message queue */
    return ipc_mq_enqueue(channel, msg);
}

ipc_status_t ipc_router_unsubscribe(ipc_channel_handle_t channel,
                                     uint32_t subscriber_pid) {
    for (uint32_t i = 0; i < IPC_MAX_ROUTER_ROUTES; i++) {
        if (g_routes[i].active && g_routes[i].channel_id == channel) {
            ipc_route_t* route = &g_routes[i];
            for (uint32_t j = 0; j < route->subscriber_count; j++) {
                if (route->subscriber_pids[j] == subscriber_pid) {
                    /* Shift remaining subscribers */
                    for (uint32_t k = j; k < route->subscriber_count - 1; k++) {
                        route->subscriber_pids[k] = route->subscriber_pids[k + 1];
                    }
                    route->subscriber_count--;
                    if (route->subscriber_count == 0) {
                        route->active = false;
                    }
                    return IPC_SUCCESS;
                }
            }
            break;
        }
    }
    return IPC_ERR_NAME_NOT_FOUND;
}
