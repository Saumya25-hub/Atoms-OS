#include "../include/bosll_api.h"

extern BOS_HANDLE BosCreateHandle(void* pObject, uint32_t accessMask);

BOS_HANDLE BosCreateIPC(const char* channelName, uint32_t bufferSize) {
    (void)channelName; (void)bufferSize;
    return BosCreateHandle((void*)5, 0);
}
