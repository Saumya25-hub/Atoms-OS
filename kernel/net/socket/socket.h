#ifndef KERNEL_SOCKET_H
#define KERNEL_SOCKET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AF_INET         2
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

#define SO_NONBLOCK     1
#define SO_RCVTIMEO     2
#define SO_SNDTIMEO     3

#define NET_OK                  0
#define NET_ERR_INVALID        -1
#define NET_ERR_WOULD_BLOCK    -11
#define NET_ERR_CLOSED         -104
#define NET_ERR_NO_RESOURCES   -105
#define NET_ERR_TIMEOUT        -110

typedef enum {
    SOCKET_STATE_FREE = 0,
    SOCKET_STATE_CREATED,
    SOCKET_STATE_BOUND,
    SOCKET_STATE_CONNECTING,
    SOCKET_STATE_CONNECTED,
    SOCKET_STATE_LISTENING,
    SOCKET_STATE_CLOSING,
    SOCKET_STATE_CLOSED,
    SOCKET_STATE_ERROR
} SocketState;

int atoms_socket(int domain, int type, int protocol);
int atoms_bind(int sock_fd, uint32_t local_ip, uint16_t local_port);
int atoms_connect(int sock_fd, uint32_t remote_ip, uint16_t remote_port);
int atoms_listen(int sock_fd, int backlog);
int atoms_accept(int sock_fd, uint32_t* remote_ip, uint16_t* remote_port);
int atoms_send(int sock_fd, const void* buf, size_t len, int flags);
int atoms_recv(int sock_fd, void* buf, size_t len, int flags);
int atoms_sendto(int sock_fd, const void* buf, size_t len, int flags, uint32_t dest_ip, uint16_t dest_port);
int atoms_recvfrom(int sock_fd, void* buf, size_t len, int flags, uint32_t* src_ip, uint16_t* src_port);
int atoms_setsockopt(int sock_fd, int level, int optname, const void* optval, size_t optlen);
int atoms_close(int sock_fd);

#endif // KERNEL_SOCKET_H
