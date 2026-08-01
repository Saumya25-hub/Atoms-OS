#include "../include/bosll_api.h"

BOS_STATUS BosProtectVirtualMemory(BOS_HANDLE hProcess, void* pBase, size_t size, uint32_t newProtect, uint32_t* pOldProtect) {
    (void)hProcess; (void)pBase; (void)size; (void)newProtect;
    if (pOldProtect) *pOldProtect = 0x04; // PAGE_READWRITE
    return BOS_SUCCESS;
}
