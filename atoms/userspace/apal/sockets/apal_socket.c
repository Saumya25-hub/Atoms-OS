/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Socket & Networking Implementation
 */

#include "apal_socket.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

#define MAX_APAL_SOCKETS 32
#define SOCK_BUF_SIZE 16384

typedef struct {
    bool in_use;
    apal_sock_type_t type;
    bool non_blocking;
    bool connected;
    char remote_ip[64];
    uint16_t remote_port;
    uint8_t rx_buf[SOCK_BUF_SIZE];
    size_t rx_head;
    size_t rx_tail;
    size_t rx_count;
} apal_sock_entry_t;

static apal_sock_entry_t g_sockets[MAX_APAL_SOCKETS];
static bool g_sockets_init = false;

static void ensure_sockets_init(void) {
    if (!g_sockets_init) {
        memset(g_sockets, 0, sizeof(g_sockets));
        g_sockets_init = true;
    }
}

apal_status_t apal_socket_create(apal_sock_type_t type, apal_socket_handle_t *out_socket) {
    if (!out_socket) return APAL_ERR_INVALID_PARAM;
    ensure_sockets_init();

    for (int i = 0; i < MAX_APAL_SOCKETS; i++) {
        if (!g_sockets[i].in_use) {
            g_sockets[i].in_use = true;
            g_sockets[i].type = type;
            g_sockets[i].non_blocking = false;
            g_sockets[i].connected = false;
            g_sockets[i].rx_head = 0;
            g_sockets[i].rx_tail = 0;
            g_sockets[i].rx_count = 0;

            *out_socket = (apal_socket_handle_t)(i + 100);
            return APAL_OK;
        }
    }
    return APAL_ERR_NO_MEMORY;
}

static apal_sock_entry_t *get_sock_entry(apal_socket_handle_t sock) {
    int idx = sock - 100;
    if (idx < 0 || idx >= MAX_APAL_SOCKETS || !g_sockets[idx].in_use) {
        return NULL;
    }
    return &g_sockets[idx];
}

apal_status_t apal_socket_connect(apal_socket_handle_t sock, const char *host_ip, uint16_t port) {
    apal_sock_entry_t *s = get_sock_entry(sock);
    if (!s || !host_ip) return APAL_ERR_INVALID_PARAM;

    strncpy(s->remote_ip, host_ip, sizeof(s->remote_ip) - 1);
    s->remote_port = port;
    s->connected = true;
    return APAL_OK;
}

apal_status_t apal_socket_bind(apal_socket_handle_t sock, const char *bind_ip, uint16_t port) {
    apal_sock_entry_t *s = get_sock_entry(sock);
    if (!s) return APAL_ERR_INVALID_PARAM;
    (void)bind_ip; (void)port;
    return APAL_OK;
}

int64_t apal_socket_send(apal_socket_handle_t sock, const void *data, size_t size, int flags) {
    apal_sock_entry_t *s = get_sock_entry(sock);
    if (!s || !data || size == 0) return -1;
    (void)flags;

    /* Loopback echo into rx buffer for self-testing / streaming */
    size_t space = SOCK_BUF_SIZE - s->rx_count;
    size_t to_write = (size < space) ? size : space;

    for (size_t i = 0; i < to_write; i++) {
        s->rx_buf[s->rx_head] = ((const uint8_t *)data)[i];
        s->rx_head = (s->rx_head + 1) % SOCK_BUF_SIZE;
    }
    s->rx_count += to_write;
    return (int64_t)to_write;
}

int64_t apal_socket_recv(apal_socket_handle_t sock, void *buffer, size_t max_size, int flags) {
    apal_sock_entry_t *s = get_sock_entry(sock);
    if (!s || !buffer || max_size == 0) return -1;
    (void)flags;

    if (s->rx_count == 0) {
        if (s->non_blocking) return 0; /* Would block */
        atoms_sys_yield();
        if (s->rx_count == 0) return 0;
    }

    size_t to_read = (s->rx_count < max_size) ? s->rx_count : max_size;
    for (size_t i = 0; i < to_read; i++) {
        ((uint8_t *)buffer)[i] = s->rx_buf[s->rx_tail];
        s->rx_tail = (s->rx_tail + 1) % SOCK_BUF_SIZE;
    }
    s->rx_count -= to_read;
    return (int64_t)to_read;
}

apal_status_t apal_socket_set_nonblocking(apal_socket_handle_t sock, bool nonblocking) {
    apal_sock_entry_t *s = get_sock_entry(sock);
    if (!s) return APAL_ERR_INVALID_PARAM;
    s->non_blocking = nonblocking;
    return APAL_OK;
}

apal_status_t apal_socket_close(apal_socket_handle_t sock) {
    apal_sock_entry_t *s = get_sock_entry(sock);
    if (!s) return APAL_ERR_INVALID_PARAM;
    s->in_use = false;
    s->connected = false;
    s->rx_count = 0;
    return APAL_OK;
}
