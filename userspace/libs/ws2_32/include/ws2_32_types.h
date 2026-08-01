#ifndef BOS_WS2_32_TYPES_H
#define BOS_WS2_32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"

typedef uint32_t u_int;

typedef uintptr_t SOCKET;
#define INVALID_SOCKET                   ((SOCKET)(~0))
#define SOCKET_ERROR                     (-1)

#define AF_UNSPEC                        0
#define AF_INET                          2
#define AF_INET6                         23

#define SOCK_STREAM                      1
#define SOCK_DGRAM                       2
#define SOCK_RAW                         3

#define IPPROTO_IP                       0
#define IPPROTO_TCP                      6
#define IPPROTO_UDP                      17

#define SOL_SOCKET                       0xFFFF

#define SO_DEBUG                         0x0001
#define SO_ACCEPTCONN                    0x0002
#define SO_REUSEADDR                     0x0004
#define SO_KEEPALIVE                     0x0008
#define SO_DONTROUTE                     0x0010
#define SO_BROADCAST                     0x0020
#define SO_USELOOPBACK                   0x0040
#define SO_LINGER                        0x0080
#define SO_OOBINLINE                     0x0100
#define SO_SNDBUF                        0x1001
#define SO_RCVBUF                        0x1002
#define SO_SNDLOWAT                      0x1003
#define SO_RCVLOWAT                      0x1004
#define SO_SNDTIMEO                      0x1005
#define SO_RCVTIMEO                      0x1006
#define SO_ERROR                         0x1007
#define SO_TYPE                          0x1008

#define TCP_NODELAY                      0x0001

#define SD_RECEIVE                       0x00
#define SD_SEND                          0x01
#define SD_BOTH                          0x02

#define FD_SETSIZE                       64

typedef struct fd_set {
    u_int fd_count;
    SOCKET fd_array[FD_SETSIZE];
} fd_set;

#define FD_CLR(fd, set) do { \
    u_int __i; \
    for (__i = 0; __i < ((fd_set*)(set))->fd_count; __i++) { \
        if (((fd_set*)(set))->fd_array[__i] == fd) { \
            while (__i < ((fd_set*)(set))->fd_count - 1) { \
                ((fd_set*)(set))->fd_array[__i] = ((fd_set*)(set))->fd_array[__i + 1]; \
                __i++; \
            } \
            ((fd_set*)(set))->fd_count--; \
            break; \
        } \
    } \
} while(0)

#define FD_SET(fd, set) do { \
    if (((fd_set*)(set))->fd_count < FD_SETSIZE) { \
        ((fd_set*)(set))->fd_array[((fd_set*)(set))->fd_count++] = (fd); \
    } \
} while(0)

#define FD_ZERO(set) (((fd_set*)(set))->fd_count = 0)

#define FD_ISSET(fd, set) __WSAFDIsSet((SOCKET)(fd), (fd_set*)(set))

struct timeval {
    long tv_sec;
    long tv_usec;
};

struct pollfd {
    SOCKET fd;
    short events;
    short revents;
};

#define POLLIN                           0x0001
#define POLLPRI                          0x0002
#define POLLOUT                          0x0004
#define POLLERR                          0x0008
#define POLLHUP                          0x0010
#define POLLNVAL                         0x0020

struct in_addr {
    union {
        struct { uint8_t s_b1, s_b2, s_b3, s_b4; } S_un_b;
        uint32_t S_addr;
    } S_un;
};
#define s_addr S_un.S_addr

struct in6_addr {
    uint8_t s6_addr[16];
};

struct sockaddr {
    uint16_t sa_family;
    char     sa_data[14];
};

struct sockaddr_in {
    int16_t        sin_family;
    uint16_t       sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct sockaddr_in6 {
    int16_t          sin6_family;
    uint16_t         sin6_port;
    uint32_t         sin6_flowinfo;
    struct in6_addr  sin6_addr;
    uint32_t         sin6_scope_id;
};

struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    size_t           ai_addrlen;
    char*            ai_canonname;
    struct sockaddr* ai_addr;
    struct addrinfo* ai_next;
};

typedef struct WSAData {
    WORD           wVersion;
    WORD           wHighVersion;
    char           szDescription[257];
    char           szSystemStatus[129];
    unsigned short iMaxSockets;
    unsigned short iMaxUdpDg;
    char*          lpVendorInfo;
} WSADATA, *LPWSADATA;

#define FD_READ_BIT                      0
#define FD_WRITE_BIT                     1
#define FD_OOB_BIT                       2
#define FD_ACCEPT_BIT                    3
#define FD_CONNECT_BIT                   4
#define FD_CLOSE_BIT                     5

#define FD_READ                          (1 << FD_READ_BIT)
#define FD_WRITE                         (1 << FD_WRITE_BIT)
#define FD_OOB                           (1 << FD_OOB_BIT)
#define FD_ACCEPT                        (1 << FD_ACCEPT_BIT)
#define FD_CONNECT                       (1 << FD_CONNECT_BIT)
#define FD_CLOSE                         (1 << FD_CLOSE_BIT)

#define FIONBIO                          0x8004667E

#endif // BOS_WS2_32_TYPES_H
