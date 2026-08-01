#include "../include/ole32_api.h"

HRESULT CoMarshalInterface(IStream* pStm, REFIID riid, IUnknown* pUnk, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags) {
    (void)pStm; (void)riid; (void)pUnk; (void)dwDestContext; (void)pvDestContext; (void)mshlflags;
    return S_OK;
}

HRESULT CoUnmarshalInterface(IStream* pStm, REFIID riid, void** ppv) {
    (void)pStm; (void)riid;
    if (!ppv) return E_POINTER;
    return CoCreateInstance(riid, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
}
