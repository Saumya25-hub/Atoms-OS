#include "../include/ole32_api.h"

HRESULT Default_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject) {
    (void)riid;
    if (!This || !ppvObject) return E_POINTER;
    *ppvObject = This;
    This->lpVtbl->AddRef(This);
    return S_OK;
}

ULONG Default_AddRef(IUnknown* This) {
    (void)This;
    return 1;
}

ULONG Default_Release(IUnknown* This) {
    (void)This;
    return 0;
}
