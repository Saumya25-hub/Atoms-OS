#include "../include/ws2_32_api.h"

int ioctlsocket(SOCKET s, long cmd, unsigned long* argp) {
    (void)s; (void)cmd; (void)argp;
    return 0;
}
