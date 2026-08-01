#include "../include/ole32_api.h"

HRESULT CoLockObjectExternal(IUnknown* pUnk, BOOL fLock, BOOL fLastUnlockReleases) {
    (void)pUnk; (void)fLock; (void)fLastUnlockReleases;
    return S_OK;
}
