#include "../include/kernel32_api.h"
#include "kernel/bar/include/bar_api.h"

HMODULE LoadLibrary(LPCSTR lpLibFileName) {
    if (!lpLibFileName) return 0;
    return (HMODULE)BAR_LoadLibrary(lpLibFileName);
}

BOOL FreeLibrary(HMODULE hLibModule) {
    if (hLibModule == 0) return false;
    return (BAR_FreeLibrary(hLibModule) == 0);
}

FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    if (hModule == 0 || !lpProcName) return NULL;
    return (FARPROC)BAR_GetProcAddress(hModule, lpProcName);
}
