#include "../include/ws2_32_api.h"

int WS2_32ValidateSocketHandle(SOCKET s) {
    return (s != INVALID_SOCKET) ? 1 : 0;
}
