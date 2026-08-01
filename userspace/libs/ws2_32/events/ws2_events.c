#include "../include/ws2_32_api.h"

HANDLE WSAEventSelect(SOCKET s, HANDLE hEventObject, long lNetworkEvents) {
    (void)s; (void)hEventObject; (void)lNetworkEvents;
    return (HANDLE)0x5000;
}
