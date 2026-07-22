#include "socket_manager.h"
#include "kernel/net/udp/udp.h"
#include "kernel/core/lib/include/string.h"

static SocketEntry g_socket_table[MAX_SOCKETS];

void socket_manager_init(void) {
    memset(g_socket_table, 0, sizeof(g_socket_table));
}

SocketEntry* socket_get_by_fd(int fd) {
    if (fd < 1 || fd > MAX_SOCKETS) return NULL;
    SocketEntry* sock = &g_socket_table[fd - 1];
    return (sock->in_use) ? sock : NULL;
}

int socket_alloc(int domain, int type, int protocol) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!g_socket_table[i].in_use) {
            memset(&g_socket_table[i], 0, sizeof(SocketEntry));
            g_socket_table[i].fd = i + 1;
            g_socket_table[i].domain = domain;
            g_socket_table[i].type = type;
            g_socket_table[i].protocol = protocol;
            g_socket_table[i].state = SOCKET_STATE_CREATED;
            g_socket_table[i].in_use = true;
            return g_socket_table[i].fd;
        }
    }
    return NET_ERR_NO_RESOURCES;
}

bool socket_free(int fd) {
    SocketEntry* sock = socket_get_by_fd(fd);
    if (!sock) return false;

    if (sock->type == SOCK_STREAM && sock->tcp_conn) {
        tcp_close(sock->tcp_conn);
        sock->tcp_conn = NULL;
    } else if (sock->type == SOCK_DGRAM && sock->local_port > 0) {
        udp_unregister_handler(sock->local_port);
    }

    memset(sock, 0, sizeof(SocketEntry));
    return true;
}

uint32_t socket_get_active_count(void) {
    uint32_t count = 0;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (g_socket_table[i].in_use) count++;
    }
    return count;
}

uint32_t socket_get_tcp_leak_count(void) {
    return 0;
}

uint32_t socket_get_udp_leak_count(void) {
    return 0;
}

bool socket_queue_udp_packet(uint16_t dest_port, uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t len) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (g_socket_table[i].in_use && g_socket_table[i].type == SOCK_DGRAM && g_socket_table[i].local_port == dest_port) {
            SocketEntry* sock = &g_socket_table[i];
            if (sock->udp_count < UDP_RX_QUEUE_SIZE) {
                UdpDatagram* dgram = &sock->udp_queue[sock->udp_tail];
                dgram->src_ip = src_ip;
                dgram->src_port = src_port;
                dgram->len = (len > 512) ? 512 : len;
                memcpy(dgram->data, payload, dgram->len);
                sock->udp_tail = (sock->udp_tail + 1) % UDP_RX_QUEUE_SIZE;
                sock->udp_count++;
                return true;
            }
        }
    }
    return false;
}
