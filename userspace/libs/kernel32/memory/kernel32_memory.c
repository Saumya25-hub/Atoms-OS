#include "../include/kernel32_api.h"

extern void* kmalloc(size_t size);
extern void  kfree(void* ptr);

LPVOID VirtualAlloc(LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
    (void)lpAddress; (void)flAllocationType; (void)flProtect;
    if (dwSize == 0) return NULL;
    return kmalloc(dwSize);
}

BOOL VirtualFree(LPVOID lpAddress, size_t dwSize, DWORD dwFreeType) {
    (void)dwSize; (void)dwFreeType;
    if (!lpAddress) return false;
    kfree(lpAddress);
    return true;
}

BOOL VirtualProtect(LPVOID lpAddress, size_t dwSize, DWORD flNewProtect, DWORD* lpflOldProtect) {
    (void)lpAddress; (void)dwSize;
    if (lpflOldProtect) *lpflOldProtect = PAGE_READWRITE;
    return (flNewProtect != 0);
}
