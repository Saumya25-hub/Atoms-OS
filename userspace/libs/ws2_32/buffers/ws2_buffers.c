#include "../include/ws2_32_api.h"

int WS2_32AllocateRingBuffer(SOCKET s, size_t size) {
    (void)s; (void)size;
    return 0;
}
