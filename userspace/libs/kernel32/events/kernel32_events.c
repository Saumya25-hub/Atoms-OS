#include "../include/kernel32_api.h"

static uint32_t g_event_counter = 1;

HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName) {
    (void)lpEventAttributes; (void)bManualReset; (void)bInitialState; (void)lpName;
    return (HANDLE)(g_event_counter++);
}

BOOL SetEvent(HANDLE hEvent) {
    return (hEvent != 0);
}

BOOL ResetEvent(HANDLE hEvent) {
    return (hEvent != 0);
}

BOOL PulseEvent(HANDLE hEvent) {
    return (hEvent != 0);
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
    (void)hHandle; (void)dwMilliseconds;
    return 0; // WAIT_OBJECT_0
}

DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) {
    (void)nCount; (void)lpHandles; (void)bWaitAll; (void)dwMilliseconds;
    return 0; // WAIT_OBJECT_0
}
