#include "../include/kernel32_api.h"

static uint32_t g_mutex_counter = 1;

HANDLE CreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) {
    (void)lpMutexAttributes; (void)bInitialOwner; (void)lpName;
    return (HANDLE)(g_mutex_counter++);
}

BOOL ReleaseMutex(HANDLE hMutex) {
    return (hMutex != 0);
}
