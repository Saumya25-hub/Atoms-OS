/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Socket & Networking Adapter (Chromium net/socket)
 */

#ifndef ATOMS_APAL_SOCKET_H
#define ATOMS_APAL_SOCKET_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int apal_socket_handle_t;
#define APAL_INVALID_SOCKET (-1)

typedef enum {
    APAL_SOCK_STREAM = 1, /* TCP */
    APAL_SOCK_DGRAM  = 2  /* UDP */
} apal_sock_type_t;

apal_status_t apal_socket_create(apal_sock_type_t type, apal_socket_handle_t *out_socket);
apal_status_t apal_socket_connect(apal_socket_handle_t sock, const char *host_ip, uint16_t port);
apal_status_t apal_socket_bind(apal_socket_handle_t sock, const char *bind_ip, uint16_t port);
int64_t apal_socket_send(apal_socket_handle_t sock, const void *data, size_t size, int flags);
int64_t apal_socket_recv(apal_socket_handle_t sock, void *buffer, size_t max_size, int flags);
apal_status_t apal_socket_set_nonblocking(apal_socket_handle_t sock, bool nonblocking);
apal_status_t apal_socket_close(apal_socket_handle_t sock);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_SOCKET_H */
