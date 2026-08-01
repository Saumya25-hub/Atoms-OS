#include "../include/ws2_32_api.h"

int WS2_32UDPEnableBroadcast(SOCKET s, int enable) {
    (void)s; (void)enable;
    return 0;
}
