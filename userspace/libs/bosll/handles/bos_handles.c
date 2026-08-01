#include "../include/bosll_api.h"

static uint64_t g_bos_handle_counter = 1;

BOS_HANDLE BosCreateHandle(void* pObject, uint32_t accessMask) {
    (void)pObject; (void)accessMask;
    return (BOS_HANDLE)(g_bos_handle_counter++);
}

BOS_STATUS BosCloseHandle(BOS_HANDLE hHandle) {
    return (hHandle != 0) ? BOS_SUCCESS : BOS_STATUS_INVALID_HANDLE;
}

BOS_STATUS BosDuplicateHandle(BOS_HANDLE hSourceProcess, BOS_HANDLE hSourceHandle, BOS_HANDLE hTargetProcess, BOS_HANDLE* phTargetHandle) {
    (void)hSourceProcess; (void)hSourceHandle; (void)hTargetProcess;
    if (phTargetHandle) *phTargetHandle = BosCreateHandle((void*)1, 0);
    return BOS_SUCCESS;
}
