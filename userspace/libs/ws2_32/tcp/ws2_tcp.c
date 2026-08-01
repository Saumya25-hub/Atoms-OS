#include "../include/ws2_32_api.h"

int WS2_32TCPSetKeepAlive(SOCKET s, int enable, int timeout_sec) {
    (void)s; (void)enable; (void)timeout_sec;
    return 0;
}
