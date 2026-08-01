#include "../include/ole32_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

HRESULT Default_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject);
ULONG Default_AddRef(IUnknown* This);
ULONG Default_Release(IUnknown* This);

static HRESULT Stream_Read(IStream* This, void* pv, ULONG cb, ULONG* pcbRead) {
    (void)This; (void)pv;
    if (pcbRead) *pcbRead = cb;
    return S_OK;
}

static HRESULT Stream_Write(IStream* This, const void* pv, ULONG cb, ULONG* pcbWritten) {
    (void)This; (void)pv;
    if (pcbWritten) *pcbWritten = cb;
    return S_OK;
}

static HRESULT Stream_Seek(IStream* This, int64_t dlibMove, DWORD dwOrigin, uint64_t* plibNewPosition) {
    (void)This; (void)dlibMove; (void)dwOrigin;
    if (plibNewPosition) *plibNewPosition = 0;
    return S_OK;
}

static HRESULT Stream_SetSize(IStream* This, uint64_t libNewSize) {
    (void)This; (void)libNewSize;
    return S_OK;
}

static const struct IStreamVtbl g_StreamVtbl = {
    (HRESULT (*)(IStream*, REFIID, void**))Default_QueryInterface,
    (ULONG (*)(IStream*))Default_AddRef,
    (ULONG (*)(IStream*))Default_Release,
    Stream_Read,
    Stream_Write,
    Stream_Seek,
    Stream_SetSize
};

HRESULT CreateStreamOnHGlobal(HANDLE hGlobal, BOOL fDeleteOnRelease, IStream** ppstm) {
    (void)hGlobal; (void)fDeleteOnRelease;
    if (!ppstm) return E_POINTER;
    IStream* stm = (IStream*)kmalloc(sizeof(IStream));
    if (!stm) return E_OUTOFMEMORY;
    stm->lpVtbl = &g_StreamVtbl;
    *ppstm = stm;
    return S_OK;
}
