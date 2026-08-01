#include "../include/kernel32_api.h"

void InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    lpCriticalSection->LockCount = 0;
    lpCriticalSection->RecursionCount = 0;
    lpCriticalSection->OwningThread = 0;
    lpCriticalSection->LockSemaphore = 0;
    lpCriticalSection->SpinCount = 0;
}

void EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    lpCriticalSection->LockCount++;
    lpCriticalSection->RecursionCount++;
}

void LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    if (lpCriticalSection->RecursionCount > 0) lpCriticalSection->RecursionCount--;
    if (lpCriticalSection->LockCount > 0) lpCriticalSection->LockCount--;
}

void DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    lpCriticalSection->LockCount = 0;
    lpCriticalSection->RecursionCount = 0;
}
