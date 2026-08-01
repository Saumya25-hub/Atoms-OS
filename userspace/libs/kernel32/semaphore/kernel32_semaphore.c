#include "../include/kernel32_api.h"

static uint32_t g_sem_counter = 1;

HANDLE CreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, int32_t lInitialCount, int32_t lMaximumCount, LPCSTR lpName) {
    (void)lpSemaphoreAttributes; (void)lInitialCount; (void)lMaximumCount; (void)lpName;
    return (HANDLE)(g_sem_counter++);
}

BOOL ReleaseSemaphore(HANDLE hSemaphore, int32_t lReleaseCount, int32_t* lpPreviousCount) {
    (void)lReleaseCount;
    if (lpPreviousCount) *lpPreviousCount = 1;
    return (hSemaphore != 0);
}
