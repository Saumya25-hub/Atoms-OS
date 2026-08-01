#include "../include/kernel32_api.h"

static LPVOID g_tls_array[64];
static bool g_tls_allocated[64];

DWORD TlsAlloc(void) {
    for (DWORD i = 0; i < 64; i++) {
        if (!g_tls_allocated[i]) {
            g_tls_allocated[i] = true;
            g_tls_array[i] = NULL;
            return i;
        }
    }
    return INFINITE;
}

LPVOID TlsGetValue(DWORD dwTlsIndex) {
    if (dwTlsIndex >= 64 || !g_tls_allocated[dwTlsIndex]) return NULL;
    return g_tls_array[dwTlsIndex];
}

BOOL TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue) {
    if (dwTlsIndex >= 64 || !g_tls_allocated[dwTlsIndex]) return false;
    g_tls_array[dwTlsIndex] = lpTlsValue;
    return true;
}

BOOL TlsFree(DWORD dwTlsIndex) {
    if (dwTlsIndex >= 64 || !g_tls_allocated[dwTlsIndex]) return false;
    g_tls_allocated[dwTlsIndex] = false;
    g_tls_array[dwTlsIndex] = NULL;
    return true;
}
