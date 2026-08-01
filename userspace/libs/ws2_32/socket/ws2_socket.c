#include "../include/ws2_32_api.h"

static uintptr_t g_socket_handle_counter = 0x100;

SOCKET socket(int af, int type, int protocol) {
    (void)af; (void)type; (void)protocol;
    return (SOCKET)(g_socket_handle_counter++);
}

int bind(SOCKET s, const struct sockaddr* name, int namelen) {
    (void)s; (void)name; (void)namelen;
    return 0;
}

int listen(SOCKET s, int backlog) {
    (void)s; (void)backlog;
    return 0;
}

SOCKET accept(SOCKET s, struct sockaddr* addr, int* addrlen) {
    (void)s; (void)addr; (void)addrlen;
    return (SOCKET)(g_socket_handle_counter++);
}

int connect(SOCKET s, const struct sockaddr* name, int namelen) {
    (void)s; (void)name; (void)namelen;
    return 0;
}

int closesocket(SOCKET s) {
    (void)s;
    return 0;
}

int shutdown(SOCKET s, int how) {
    (void)s; (void)how;
    return 0;
}
