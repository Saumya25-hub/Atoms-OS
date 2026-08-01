#include "../include/advapi32_api.h"

BOOL AdjustTokenPrivileges(HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength) {
    (void)TokenHandle; (void)DisableAllPrivileges; (void)NewState; (void)BufferLength; (void)PreviousState; (void)ReturnLength;
    return TRUE;
}

BOOL LookupPrivilegeValueA(LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid) {
    (void)lpSystemName; (void)lpName;
    if (lpLuid) { lpLuid->LowPart = 20; lpLuid->HighPart = 0; }
    return TRUE;
}
