#include "../include/ws2_32_api.h"

int WS2_32ValidateSocketSecurity(SOCKET s, uint32_t required_capability) {
    (void)s; (void)required_capability;
    return 1;
}
