#include "../include/bosll_api.h"

BOS_STATUS BosGetThreadContext(BOS_HANDLE hThread, BOS_CPU_CONTEXT* pCtx) {
    (void)hThread;
    if (!pCtx) return BOS_STATUS_UNSUCCESSFUL;
    pCtx->Rip = 0x401000;
    pCtx->Rsp = 0x7FFF0000;
    pCtx->Rflags = 0x202;
    return BOS_SUCCESS;
}

BOS_STATUS BosSetThreadContext(BOS_HANDLE hThread, const BOS_CPU_CONTEXT* pCtx) {
    (void)hThread; (void)pCtx;
    return BOS_SUCCESS;
}
