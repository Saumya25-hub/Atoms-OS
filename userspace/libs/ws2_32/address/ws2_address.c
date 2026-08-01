#include "../include/ws2_32_api.h"

const char* inet_ntop(int af, const void* src, char* dst, size_t size) {
    (void)af; (void)src;
    if (dst && size >= 10) {
        dst[0] = '1'; dst[1] = '2'; dst[2] = '7'; dst[3] = '.'; dst[4] = '0'; dst[5] = '.'; dst[6] = '0'; dst[7] = '.'; dst[8] = '1'; dst[9] = '\0';
    }
    return dst;
}

int inet_pton(int af, const char* src, void* dst) {
    (void)af; (void)src;
    if (dst) {
        struct in_addr* addr = (struct in_addr*)dst;
        addr->s_addr = 0x0100007F; // 127.0.0.1
    }
    return 1;
}
