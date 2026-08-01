#include "../include/bosll_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

BOS_STATUS BosAllocateVirtualMemory(BOS_HANDLE hProcess, void** ppBase, size_t size, uint32_t allocType, uint32_t protect) {
    (void)hProcess; (void)allocType; (void)protect;
    if (!ppBase || size == 0) return BOS_STATUS_NO_MEMORY;
    *ppBase = kmalloc(size);
    return (*ppBase != NULL) ? BOS_SUCCESS : BOS_STATUS_NO_MEMORY;
}

BOS_STATUS BosFreeVirtualMemory(BOS_HANDLE hProcess, void* pBase, size_t size, uint32_t freeType) {
    (void)hProcess; (void)size; (void)freeType;
    if (pBase) kfree(pBase);
    return BOS_SUCCESS;
}
