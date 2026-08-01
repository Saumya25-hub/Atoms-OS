#ifndef BOS_OLE32_TYPES_H
#define BOS_OLE32_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"

typedef uint32_t ULONG;
typedef int32_t HRESULT;

#define S_OK                             ((HRESULT)0x00000000L)
#define S_FALSE                          ((HRESULT)0x00000001L)
#define E_UNEXPECTED                     ((HRESULT)0x80000001L)
#define E_NOTIMPL                        ((HRESULT)0x80004001L)
#define E_OUTOFMEMORY                    ((HRESULT)0x8007000EL)
#define E_INVALIDARG                     ((HRESULT)0x80070057L)
#define E_NOINTERFACE                    ((HRESULT)0x80004002L)
#define E_POINTER                        ((HRESULT)0x80004003L)
#define E_HANDLE                         ((HRESULT)0x80070006L)
#define E_ABORT                          ((HRESULT)0x80004004L)
#define E_FAIL                           ((HRESULT)0x80004005L)
#define E_ACCESSDENIED                   ((HRESULT)0x80070005L)
#define CO_E_NOTINITIALIZED              ((HRESULT)0x800401F0L)

#define SUCCEEDED(hr)                    (((HRESULT)(hr)) >= 0)
#define FAILED(hr)                       (((HRESULT)(hr)) < 0)

typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} GUID, CLSID, IID;

typedef const GUID* REFGUID;
typedef const GUID* REFCLSID;
typedef const GUID* REFIID;

typedef struct _POINTL {
    int32_t x;
    int32_t y;
} POINTL;

#define COINIT_APARTMENTTHREADED         0x2
#define COINIT_MULTITHREADED             0x0
#define COINIT_DISABLE_OLE1DDE           0x4
#define COINIT_SPEED_OVER_MEMORY         0x8

#define CLSCTX_INPROC_SERVER             0x1
#define CLSCTX_INPROC_HANDLER            0x2
#define CLSCTX_LOCAL_SERVER              0x4
#define CLSCTX_REMOTE_SERVER             0x10
#define CLSCTX_ALL                       (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)

// Forward Interface Declarations & Vtables
struct IUnknownVtbl;
typedef struct IUnknown {
    const struct IUnknownVtbl* lpVtbl;
} IUnknown;

struct IUnknownVtbl {
    HRESULT (*QueryInterface)(IUnknown* This, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(IUnknown* This);
    ULONG   (*Release)(IUnknown* This);
};

struct IClassFactoryVtbl;
typedef struct IClassFactory {
    const struct IClassFactoryVtbl* lpVtbl;
} IClassFactory;

struct IClassFactoryVtbl {
    HRESULT (*QueryInterface)(IClassFactory* This, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(IClassFactory* This);
    ULONG   (*Release)(IClassFactory* This);
    HRESULT (*CreateInstance)(IClassFactory* This, IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
    HRESULT (*LockServer)(IClassFactory* This, BOOL fLock);
};

struct IStreamVtbl;
typedef struct IStream {
    const struct IStreamVtbl* lpVtbl;
} IStream;

struct IStreamVtbl {
    HRESULT (*QueryInterface)(IStream* This, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(IStream* This);
    ULONG   (*Release)(IStream* This);
    HRESULT (*Read)(IStream* This, void* pv, ULONG cb, ULONG* pcbRead);
    HRESULT (*Write)(IStream* This, const void* pv, ULONG cb, ULONG* pcbWritten);
    HRESULT (*Seek)(IStream* This, int64_t dlibMove, DWORD dwOrigin, uint64_t* plibNewPosition);
    HRESULT (*SetSize)(IStream* This, uint64_t libNewSize);
};

struct IStorageVtbl;
typedef struct IStorage {
    const struct IStorageVtbl* lpVtbl;
} IStorage;

struct IStorageVtbl {
    HRESULT (*QueryInterface)(IStorage* This, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(IStorage* This);
    ULONG   (*Release)(IStorage* This);
    HRESULT (*CreateStream)(IStorage* This, const char* pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream** ppstm);
    HRESULT (*OpenStream)(IStorage* This, const char* pwcsName, void* reserved1, DWORD grfMode, DWORD reserved2, IStream** ppstm);
};

struct IDataObjectVtbl;
typedef struct IDataObject {
    const struct IDataObjectVtbl* lpVtbl;
} IDataObject;

struct IDataObjectVtbl {
    HRESULT (*QueryInterface)(IDataObject* This, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(IDataObject* This);
    ULONG   (*Release)(IDataObject* This);
    HRESULT (*GetData)(IDataObject* This, void* pformatetcIn, void* pmedium);
    HRESULT (*SetData)(IDataObject* This, void* pformatetc, void* pmedium, BOOL fRelease);
};

struct IDropSourceVtbl;
typedef struct IDropSource {
    const struct IDropSourceVtbl* lpVtbl;
} IDropSource;

struct IDropSourceVtbl {
    HRESULT (*QueryInterface)(IDropSource* This, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(IDropSource* This);
    ULONG   (*Release)(IDropSource* This);
    HRESULT (*QueryContinueDrag)(IDropSource* This, BOOL fEscapePressed, DWORD grfKeyState);
    HRESULT (*GiveFeedback)(IDropSource* This, DWORD dwEffect);
};

struct IDropTargetVtbl;
typedef struct IDropTarget {
    const struct IDropTargetVtbl* lpVtbl;
} IDropTarget;

struct IDropTargetVtbl {
    HRESULT (*QueryInterface)(IDropTarget* This, REFIID riid, void** ppvObject);
    ULONG   (*AddRef)(IDropTarget* This);
    ULONG   (*Release)(IDropTarget* This);
    HRESULT (*DragEnter)(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    HRESULT (*DragOver)(IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    HRESULT (*DragLeave)(IDropTarget* This);
    HRESULT (*Drop)(IDropTarget* This, IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
};

#endif // BOS_OLE32_TYPES_H
