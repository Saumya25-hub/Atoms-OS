#include "../include/ws2_32_api.h"

int send(SOCKET s, const char* buf, int len, int flags) {
    (void)s; (void)buf; (void)flags;
    return len;
}

int recv(SOCKET s, char* buf, int len, int flags) {
    (void)s; (void)flags;
    if (buf && len > 0) buf[0] = '\0';
    return len;
}

int sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) {
    (void)s; (void)buf; (void)flags; (void)to; (void)tolen;
    return len;
}

int recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen) {
    (void)s; (void)flags; (void)from; (void)fromlen;
    if (buf && len > 0) buf[0] = '\0';
    return len;
}
