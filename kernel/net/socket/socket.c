#include "socket.h"
#include "socket_manager.h"
#include "kernel/net/tcp/tcp.h"
#include "kernel/net/udp/udp.h"
#include "kernel/net/netif.h"
#include "kernel/net/netif/net_service.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern uint32_t timer_get_ticks(void);

int atoms_socket(int domain, int type, int protocol) {
    if (domain != AF_INET) return NET_ERR_INVALID;
    if (type != SOCK_STREAM && type != SOCK_DGRAM) return NET_ERR_INVALID;

    if (protocol == 0) {
        protocol = (type == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP;
    }

    return socket_alloc(domain, type, protocol);
}

int atoms_bind(int sock_fd, uint32_t local_ip, uint16_t local_port) {
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock) return NET_ERR_INVALID;

    sock->local_ip = local_ip;
    sock->local_port = local_port;
    sock->state = SOCKET_STATE_BOUND;
    return NET_OK;
}

int atoms_connect(int sock_fd, uint32_t remote_ip, uint16_t remote_port) {
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock || sock->type != SOCK_STREAM) return NET_ERR_INVALID;

    sock->remote_ip = remote_ip;
    sock->remote_port = remote_port;
    sock->state = SOCKET_STATE_CONNECTING;

    TcpConnection* conn = NULL;
    if (!tcp_connect(remote_ip, remote_port, &conn) || !conn) {
        sock->state = SOCKET_STATE_ERROR;
        return NET_ERR_TIMEOUT;
    }

    sock->tcp_conn = conn;
    sock->local_ip = conn->local_ip;
    sock->local_port = conn->local_port;
    sock->state = SOCKET_STATE_CONNECTED;
    return NET_OK;
}

int atoms_send(int sock_fd, const void* buf, size_t len, int flags) {
    (void)flags;
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock || sock->type != SOCK_STREAM || !sock->tcp_conn) return NET_ERR_INVALID;
    if (sock->state != SOCKET_STATE_CONNECTED) return NET_ERR_CLOSED;

    return tcp_send(sock->tcp_conn, buf, len);
}

int atoms_recv(int sock_fd, void* buf, size_t len, int flags) {
    (void)flags;
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock || sock->type != SOCK_STREAM || !sock->tcp_conn) return NET_ERR_INVALID;

    size_t avail = tcp_available(sock->tcp_conn);
    if (avail == 0) {
        if (sock->tcp_conn->fin_received || sock->tcp_conn->state == TCP_STATE_CLOSED) {
            return 0; // Peer closed connection
        }
        if (sock->non_blocking) {
            return NET_ERR_WOULD_BLOCK;
        }

        // Bounded Wait for RX
        uint32_t start_tick = timer_get_ticks();
        uint32_t timeout_ticks = (sock->recv_timeout_ms > 0) ? (sock->recv_timeout_ms / 10) : 250;

        while ((timer_get_ticks() - start_tick) < timeout_ticks) {
            net_service_poll();
            if (tcp_available(sock->tcp_conn) > 0) break;
            if (sock->tcp_conn->fin_received || sock->tcp_conn->state == TCP_STATE_CLOSED) return 0;
        }

        if (tcp_available(sock->tcp_conn) == 0) {
            return NET_ERR_WOULD_BLOCK;
        }
    }

    return tcp_recv(sock->tcp_conn, buf, len);
}

int atoms_sendto(int sock_fd, const void* buf, size_t len, int flags, uint32_t dest_ip, uint16_t dest_port) {
    (void)flags;
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock || sock->type != SOCK_DGRAM) return NET_ERR_INVALID;

    if (sock->local_port == 0) {
        sock->local_port = 50000 + (sock_fd * 100);
    }

    NetInterface* netif = netif_get_default();
    uint32_t src_ip = netif ? netif->ip_addr : 0;

    bool ok = udp_send(src_ip, dest_ip, sock->local_port, dest_port, buf, (uint16_t)len);
    return ok ? (int)len : NET_ERR_INVALID;
}

int atoms_recvfrom(int sock_fd, void* buf, size_t len, int flags, uint32_t* src_ip, uint16_t* src_port) {
    (void)flags;
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock || sock->type != SOCK_DGRAM) return NET_ERR_INVALID;

    if (sock->udp_count == 0) {
        if (sock->non_blocking) return NET_ERR_WOULD_BLOCK;
        net_service_poll();
        if (sock->udp_count == 0) return NET_ERR_WOULD_BLOCK;
    }

    UdpDatagram* dgram = &sock->udp_queue[sock->udp_head];
    size_t copy_len = (len < dgram->len) ? len : dgram->len;
    memcpy(buf, dgram->data, copy_len);

    if (src_ip) *src_ip = dgram->src_ip;
    if (src_port) *src_port = dgram->src_port;

    sock->udp_head = (sock->udp_head + 1) % UDP_RX_QUEUE_SIZE;
    sock->udp_count--;

    return (int)copy_len;
}

int atoms_setsockopt(int sock_fd, int level, int optname, const void* optval, size_t optlen) {
    (void)level; (void)optlen;
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock) return NET_ERR_INVALID;

    if (optname == SO_NONBLOCK) {
        sock->non_blocking = *(const bool*)optval;
    } else if (optname == SO_RCVTIMEO) {
        sock->recv_timeout_ms = *(const uint32_t*)optval;
    }
    return NET_OK;
}

int atoms_close(int sock_fd) {
    SocketEntry* sock = socket_get_by_fd(sock_fd);
    if (!sock) return NET_ERR_INVALID;

    socket_free(sock_fd);
    return NET_OK;
}
