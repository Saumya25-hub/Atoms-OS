#include "../include/bosll_api.h"

extern BOS_HANDLE BosCreateHandle(void* pObject, uint32_t accessMask);

BOS_HANDLE BosCreateEvent(bool manualReset, bool initialState) {
    (void)manualReset; (void)initialState;
    return BosCreateHandle((void*)3, 0);
}

BOS_HANDLE BosCreateMutex(bool initialOwner) {
    (void)initialOwner;
    return BosCreateHandle((void*)4, 0);
}

BOS_STATUS BosWaitObject(BOS_HANDLE hObject, uint32_t timeoutMs) {
    (void)hObject; (void)timeoutMs;
    return BOS_SUCCESS;
}
