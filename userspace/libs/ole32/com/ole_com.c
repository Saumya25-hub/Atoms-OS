#include "../include/ole32_api.h"

static DWORD g_co_init_flags = 0;

HRESULT CoInitialize(void* pvReserved) {
    return CoInitializeEx(pvReserved, COINIT_APARTMENTTHREADED);
}

HRESULT CoInitializeEx(void* pvReserved, DWORD dwCoInit) {
    (void)pvReserved;
    g_co_init_flags = dwCoInit;
    return S_OK;
}

void CoUninitialize(void) {
    g_co_init_flags = 0;
}
