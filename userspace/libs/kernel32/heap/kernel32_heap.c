#include "../include/kernel32_api.h"

extern void* kmalloc(size_t size);
extern void  kfree(void* ptr);

static uint32_t g_heap_counter = 1;

HANDLE HeapCreate(DWORD flOptions, size_t dwInitialSize, size_t dwMaximumSize) {
    (void)flOptions; (void)dwInitialSize; (void)dwMaximumSize;
    return (HANDLE)(g_heap_counter++);
}

LPVOID HeapAlloc(HANDLE hHeap, DWORD dwFlags, size_t dwBytes) {
    (void)hHeap; (void)dwFlags;
    if (dwBytes == 0) return NULL;
    return kmalloc(dwBytes);
}

LPVOID HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, size_t dwBytes) {
    (void)hHeap; (void)dwFlags;
    if (!lpMem) return HeapAlloc(hHeap, dwFlags, dwBytes);
    void* new_ptr = kmalloc(dwBytes);
    if (new_ptr && lpMem) {
        kfree(lpMem);
    }
    return new_ptr;
}

BOOL HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem) {
    (void)hHeap; (void)dwFlags;
    if (!lpMem) return false;
    kfree(lpMem);
    return true;
}

BOOL HeapDestroy(HANDLE hHeap) {
    return (hHeap != 0);
}
