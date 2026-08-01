#include "../include/ws2_32_api.h"

static int g_wsa_last_error = 0;

int WSAGetLastError(void) {
    return g_wsa_last_error;
}

void WSASetLastError(int err) {
    g_wsa_last_error = err;
}
