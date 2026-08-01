#include "../include/bosll_api.h"

BOS_STATUS BosCreateThread(BOS_HANDLE hProcess, void* entryPoint, void* param, BOS_THREAD_INFO* tInfo, BOS_HANDLE* phThread) {
    (void)hProcess; (void)entryPoint; (void)param;
    if (tInfo) { tInfo->ThreadId = 200; tInfo->ProcessId = 100; tInfo->StackBase = 0x7FFF0000; tInfo->StackLimit = 0x7FFE0000; }
    if (phThread) *phThread = BosCreateHandle((void*)2, 0);
    return BOS_SUCCESS;
}

BOS_STATUS BosExitThread(uint32_t exitCode) {
    (void)exitCode;
    return BOS_SUCCESS;
}

BOS_HANDLE BosGetCurrentThread(void) {
    return (BOS_HANDLE)0xFFFFFFFE;
}
