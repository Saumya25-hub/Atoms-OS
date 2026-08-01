#include "../include/ole32_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

HRESULT Default_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject);
ULONG Default_AddRef(IUnknown* This);
ULONG Default_Release(IUnknown* This);

static HRESULT Storage_CreateStream(IStorage* This, const char* pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream** ppstm) {
    (void)This; (void)pwcsName; (void)grfMode; (void)reserved1; (void)reserved2;
    return CreateStreamOnHGlobal(0, true, ppstm);
}

static HRESULT Storage_OpenStream(IStorage* This, const char* pwcsName, void* reserved1, DWORD grfMode, DWORD reserved2, IStream** ppstm) {
    (void)This; (void)pwcsName; (void)reserved1; (void)grfMode; (void)reserved2;
    return CreateStreamOnHGlobal(0, true, ppstm);
}

static const struct IStorageVtbl g_StorageVtbl = {
    (HRESULT (*)(IStorage*, REFIID, void**))Default_QueryInterface,
    (ULONG (*)(IStorage*))Default_AddRef,
    (ULONG (*)(IStorage*))Default_Release,
    Storage_CreateStream,
    Storage_OpenStream
};

HRESULT StgCreateStorageEx(const char* pwcsName, DWORD grfMode, DWORD stgfmt, DWORD grfAttrs, void* pStgOptions, void* pSecurityDescriptor, REFIID riid, void** ppObjectOpen) {
    (void)pwcsName; (void)grfMode; (void)stgfmt; (void)grfAttrs; (void)pStgOptions; (void)pSecurityDescriptor; (void)riid;
    if (!ppObjectOpen) return E_POINTER;
    IStorage* stg = (IStorage*)kmalloc(sizeof(IStorage));
    if (!stg) return E_OUTOFMEMORY;
    stg->lpVtbl = &g_StorageVtbl;
    *ppObjectOpen = stg;
    return S_OK;
}

HRESULT StgOpenStorage(const char* pwcsName, IStorage* pstgPriority, DWORD grfMode, void* snbExclude, DWORD reserved, IStorage** ppstgOpen) {
    (void)pstgPriority; (void)snbExclude; (void)reserved;
    return StgCreateStorageEx(pwcsName, grfMode, 0, 0, NULL, NULL, &IID_IStorage, (void**)ppstgOpen);
}
