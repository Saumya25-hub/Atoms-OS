#include "../include/ole32_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

HRESULT Default_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject);
ULONG Default_AddRef(IUnknown* This);
ULONG Default_Release(IUnknown* This);

static HRESULT Factory_CreateInstance(IClassFactory* This, IUnknown* pUnkOuter, REFIID riid, void** ppvObject) {
    (void)This; (void)pUnkOuter; (void)riid;
    if (!ppvObject) return E_POINTER;
    IUnknown* pUnk = (IUnknown*)kmalloc(sizeof(IUnknown) + sizeof(struct IUnknownVtbl));
    if (!pUnk) return E_OUTOFMEMORY;
    struct IUnknownVtbl* vtbl = (struct IUnknownVtbl*)((char*)pUnk + sizeof(IUnknown));
    vtbl->QueryInterface = Default_QueryInterface;
    vtbl->AddRef = Default_AddRef;
    vtbl->Release = Default_Release;
    pUnk->lpVtbl = vtbl;
    *ppvObject = pUnk;
    return S_OK;
}

static HRESULT Factory_LockServer(IClassFactory* This, BOOL fLock) {
    (void)This; (void)fLock;
    return S_OK;
}

static const struct IClassFactoryVtbl g_FactoryVtbl = {
    (HRESULT (*)(IClassFactory*, REFIID, void**))Default_QueryInterface,
    (ULONG (*)(IClassFactory*))Default_AddRef,
    (ULONG (*)(IClassFactory*))Default_Release,
    Factory_CreateInstance,
    Factory_LockServer
};

static IClassFactory g_DefaultFactory = { &g_FactoryVtbl };

HRESULT CoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, void** ppv) {
    (void)rclsid; (void)pUnkOuter; (void)dwClsContext;
    if (!ppv) return E_POINTER;
    return g_DefaultFactory.lpVtbl->CreateInstance(&g_DefaultFactory, pUnkOuter, riid, ppv);
}

HRESULT CoRegisterClassObject(REFCLSID rclsid, IUnknown* pUnk, DWORD dwClsContext, DWORD flags, DWORD* lpdwRegister) {
    (void)rclsid; (void)pUnk; (void)dwClsContext; (void)flags;
    if (lpdwRegister) *lpdwRegister = 0x100;
    return S_OK;
}

HRESULT CoRevokeClassObject(DWORD dwRegister) {
    (void)dwRegister;
    return S_OK;
}
