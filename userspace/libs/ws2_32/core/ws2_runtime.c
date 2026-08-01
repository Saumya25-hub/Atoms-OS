#include "../include/ws2_32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_ws2_32_initialized = false;

int32_t WS2_32Initialize(void) {
    if (g_ws2_32_initialized) return 1;
    g_ws2_32_initialized = true;
    display_print("[WS2_32] WS2_32.sll Network Runtime & Winsock Framework V1.0 Initialized\n");
    return 1;
}

int32_t WS2_32Shutdown(void) {
    if (!g_ws2_32_initialized) return 1;
    g_ws2_32_initialized = false;
    display_print("[WS2_32] WS2_32.sll Subsystem Shutdown Cleanly\n");
    return 1;
}

int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData) {
    (void)wVersionRequested;
    if (lpWSAData) {
        lpWSAData->wVersion = 0x0202;
        lpWSAData->wHighVersion = 0x0202;
        lpWSAData->szDescription[0] = 'W'; lpWSAData->szDescription[1] = 'i'; lpWSAData->szDescription[2] = 'n'; lpWSAData->szDescription[3] = 's'; lpWSAData->szDescription[4] = 'o'; lpWSAData->szDescription[5] = 'c'; lpWSAData->szDescription[6] = 'k'; lpWSAData->szDescription[7] = '\0';
        lpWSAData->szSystemStatus[0] = 'R'; lpWSAData->szSystemStatus[1] = 'e'; lpWSAData->szSystemStatus[2] = 'a'; lpWSAData->szSystemStatus[3] = 'd'; lpWSAData->szSystemStatus[4] = 'y'; lpWSAData->szSystemStatus[5] = '\0';
        lpWSAData->iMaxSockets = 256;
        lpWSAData->iMaxUdpDg = 65535;
    }
    return 0;
}

int WSACleanup(void) {
    return 0;
}
