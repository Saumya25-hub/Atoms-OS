#include "../include/bosll_api.h"

BOS_STATUS BosCreateProcess(const char* imagePath, const char* cmdLine, BOS_PROCESS_INFO* pInfo, BOS_HANDLE* phProcess) {
    (void)imagePath; (void)cmdLine;
    if (pInfo) { pInfo->ProcessId = 100; pInfo->ParentProcessId = 1; pInfo->Flags = 0; }
    if (phProcess) *phProcess = BosCreateHandle((void*)1, 0);
    return BOS_SUCCESS;
}

BOS_STATUS BosTerminateProcess(BOS_HANDLE hProcess, uint32_t exitCode) {
    (void)hProcess; (void)exitCode;
    return BOS_SUCCESS;
}

BOS_HANDLE BosGetCurrentProcess(void) {
    return (BOS_HANDLE)0xFFFFFFFF;
}
