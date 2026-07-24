/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_manager.c — Subsystem Lifecycle Orchestrator Implementation
 *
 * Initializes all sub-engines in correct dependency order.
 * Also implements the public API facade that delegates to sub-engines.
 */

#include "kernel/ipc/core/ipc_manager.h"
#include "kernel/ipc/debug/ipc_debug.h"
#include "kernel/ipc/sync/ipc_sync.h"
#include "kernel/ipc/permissions/ipc_permissions.h"
#include "kernel/ipc/channels/channel_manager.h"
#include "kernel/ipc/message/message_queue.h"
#include "kernel/ipc/pipes/pipe_engine.h"
#include "kernel/ipc/ports/port_manager.h"
#include "kernel/ipc/shared_memory/shm_manager.h"
#include "kernel/ipc/router/ipc_router.h"

static bool g_ipc_initialized = false;

/* ============================================================
 * Subsystem Lifecycle
 * ============================================================ */

ipc_status_t bos_ipc_init(void) {
    if (g_ipc_initialized) return IPC_SUCCESS;

    ipc_debug_init();
    ipc_debug_log(IPC_LOG_INFO, "IPC_MGR",
        "Initializing BOS OS Phase 2 Production IPC & Shared Memory Engine...");

    /* Initialize in dependency order */
    ipc_sync_init();
    ipc_permissions_init();
    ipc_channel_manager_init();
    ipc_message_queue_init();
    ipc_pipe_engine_init();
    ipc_port_manager_init();
    ipc_shm_manager_init();
    ipc_router_init();

    g_ipc_initialized = true;
    ipc_debug_log(IPC_LOG_INFO, "IPC_MGR",
        "BOS OS IPC & Shared Memory Subsystem Ready & Active");

    return IPC_SUCCESS;
}

ipc_status_t bos_ipc_shutdown(void) {
    g_ipc_initialized = false;
    return IPC_SUCCESS;
}

/* ============================================================
 * Public API Facade — Channels
 * ============================================================ */

ipc_status_t bos_ipc_create_channel(const char* name, uint32_t flags,
                                     ipc_channel_handle_t* out_handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_channel_create(name, flags, 0 /* kernel PID */, out_handle);
}

ipc_status_t bos_ipc_connect(const char* name,
                              ipc_channel_handle_t* out_handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_channel_connect_by_name(name, 0, out_handle);
}

static void ipc_msg_memcpy(void* dst, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}

ipc_status_t bos_ipc_send(ipc_channel_handle_t channel,
                           const void* data, uint32_t size, uint32_t flags) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    if (!data || size == 0) return IPC_ERR_NULL_POINTER;
    if (size > IPC_MAX_MESSAGE_SIZE) return IPC_ERR_INVALID_SIZE;

    ipc_channel_t* ch = ipc_channel_get(channel);
    if (!ch) return IPC_ERR_INVALID_HANDLE;

    ipc_message_t msg;
    msg.header.src_pid         = ch->owner_pid;
    msg.header.dst_pid         = ch->peer_pid;
    msg.header.channel_id      = ch->id;
    msg.header.msg_type        = 0;
    msg.header.payload_size    = size;
    msg.header.sequence_number = ch->messages_sent;
    msg.header.flags           = flags;
    msg.header.reserved        = 0;

    ipc_msg_memcpy(msg.payload, data, size);

    ipc_status_t result = ipc_mq_enqueue(channel, &msg);
    if (result == IPC_SUCCESS) {
        ch->messages_sent++;
    }
    return result;
}

ipc_status_t bos_ipc_receive(ipc_channel_handle_t channel,
                              void* buffer, uint32_t buffer_size,
                              uint32_t* out_size, uint32_t flags) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    if (!buffer || !out_size) return IPC_ERR_NULL_POINTER;

    ipc_channel_t* ch = ipc_channel_get(channel);
    if (!ch) return IPC_ERR_INVALID_HANDLE;

    ipc_message_t msg;
    ipc_status_t result = ipc_mq_dequeue(channel, &msg, flags);
    if (result != IPC_SUCCESS) return result;

    uint32_t copy_size = msg.header.payload_size;
    if (copy_size > buffer_size) copy_size = buffer_size;

    ipc_msg_memcpy(buffer, msg.payload, copy_size);
    *out_size = copy_size;
    ch->messages_received++;

    return IPC_SUCCESS;
}

ipc_status_t bos_ipc_close(ipc_channel_handle_t channel) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_channel_close(channel);
}

/* ============================================================
 * Public API Facade — Shared Memory
 * ============================================================ */

ipc_status_t bos_shm_create(const char* name, uint32_t size,
                             uint32_t flags, ipc_shm_handle_t* out_handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_shm_create(name, size, flags, 0, out_handle);
}

ipc_status_t bos_shm_open(const char* name, uint32_t flags,
                            ipc_shm_handle_t* out_handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_shm_open_by_name(name, flags, out_handle);
}

ipc_status_t bos_shm_map(ipc_shm_handle_t handle, uint32_t pid,
                           uint32_t perm, void** out_addr) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_shm_map_to_process(handle, pid, perm, out_addr);
}

ipc_status_t bos_shm_unmap(ipc_shm_handle_t handle, uint32_t pid) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_shm_unmap_from_process(handle, pid);
}

ipc_status_t bos_shm_close(ipc_shm_handle_t handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_shm_close_handle(handle);
}

ipc_status_t bos_shm_destroy(ipc_shm_handle_t handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_shm_destroy_object(handle);
}

/* ============================================================
 * Public API Facade — Pipes
 * ============================================================ */

ipc_status_t bos_pipe_create(uint32_t reader_pid, uint32_t writer_pid,
                              ipc_pipe_handle_t* out_handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_pipe_create(reader_pid, writer_pid, out_handle);
}

ipc_status_t bos_pipe_write(ipc_pipe_handle_t handle,
                             const void* data, uint32_t size,
                             uint32_t* out_written) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_pipe_write(handle, data, size, out_written);
}

ipc_status_t bos_pipe_read(ipc_pipe_handle_t handle,
                            void* buffer, uint32_t buffer_size,
                            uint32_t* out_read) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_pipe_read(handle, buffer, buffer_size, out_read);
}

ipc_status_t bos_pipe_close(ipc_pipe_handle_t handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_pipe_close(handle);
}

/* ============================================================
 * Public API Facade — Ports
 * ============================================================ */

ipc_status_t bos_port_create(const char* name, uint32_t owner_pid,
                              ipc_port_handle_t* out_handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_port_create(name, owner_pid, out_handle);
}

ipc_status_t bos_port_bind(ipc_port_handle_t port,
                            ipc_channel_handle_t channel) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_port_bind(port, channel);
}

ipc_status_t bos_port_lookup(const char* name,
                              ipc_port_handle_t* out_handle) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_port_lookup(name, out_handle);
}

ipc_status_t bos_port_close(ipc_port_handle_t port) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_port_close(port);
}

/* ============================================================
 * Public API Facade — Router
 * ============================================================ */

ipc_status_t bos_ipc_broadcast(ipc_channel_handle_t channel,
                                const void* data, uint32_t size) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    if (!data || size == 0) return IPC_ERR_NULL_POINTER;
    if (size > IPC_MAX_MESSAGE_SIZE) return IPC_ERR_INVALID_SIZE;

    ipc_message_t msg;
    msg.header.src_pid         = 0;
    msg.header.dst_pid         = 0; /* broadcast */
    msg.header.channel_id      = channel;
    msg.header.msg_type        = 0;
    msg.header.payload_size    = size;
    msg.header.sequence_number = 0;
    msg.header.flags           = IPC_CHANNEL_BROADCAST;
    msg.header.reserved        = 0;
    ipc_msg_memcpy(msg.payload, data, size);

    return ipc_router_broadcast(channel, &msg);
}

ipc_status_t bos_ipc_subscribe(ipc_channel_handle_t channel,
                                uint32_t subscriber_pid) {
    if (!g_ipc_initialized) return IPC_ERR_NOT_INITIALIZED;
    return ipc_router_subscribe(channel, subscriber_pid);
}
