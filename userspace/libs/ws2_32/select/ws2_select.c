#include "../include/ws2_32_api.h"

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout) {
    (void)nfds; (void)writefds; (void)exceptfds; (void)timeout;
    return (readfds) ? readfds->fd_count : 0;
}

int __WSAFDIsSet(SOCKET s, fd_set* set) {
    if (!set) return 0;
    for (u_int i = 0; i < set->fd_count; i++) {
        if (set->fd_array[i] == s) return 1;
    }
    return 0;
}
