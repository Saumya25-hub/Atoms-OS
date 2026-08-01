#include "../include/ws2_32_api.h"

int setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen) {
    (void)s; (void)level; (void)optname; (void)optval; (void)optlen;
    return 0;
}

int getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen) {
    (void)s; (void)level; (void)optname;
    if (optval && optlen && *optlen >= 4) {
        *(int*)optval = 1;
    }
    return 0;
}
