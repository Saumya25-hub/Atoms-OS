#ifndef KERNEL_SOCKET_MANAGER_H
#define KERNEL_SOCKET_MANAGER_H

#include "socket.h"
#include "kernel/net/tcp/tcp.h"

#define MAX_SOCKETS 16
#define UDP_RX_QUEUE_SIZE 8

typedef struct {
    uint32_t src_ip;
    uint16_t src_port;
    uint8_t  data[512];
    uint16_t len;
} UdpDatagram;

typedef struct {
    int          fd;
    int          domain;
    int          type;
    int          protocol;
    SocketState  state;

    uint32_t     local_ip;
    uint16_t     local_port;
    uint32_t     remote_ip;
    uint16_t     remote_port;

    TcpConnection* tcp_conn;

    UdpDatagram  udp_queue[UDP_RX_QUEUE_SIZE];
    size_t       udp_head;
    size_t       udp_tail;
    size_t       udp_count;

    bool         non_blocking;
    uint32_t     recv_timeout_ms;

    bool         in_use;
} SocketEntry;

void socket_manager_init(void);
SocketEntry* socket_get_by_fd(int fd);
int socket_alloc(int domain, int type, int protocol);
bool socket_free(int fd);

uint32_t socket_get_active_count(void);
uint32_t socket_get_tcp_leak_count(void);
uint32_t socket_get_udp_leak_count(void);

bool socket_queue_udp_packet(uint16_t dest_port, uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t len);

#endif // KERNEL_SOCKET_MANAGER_H
