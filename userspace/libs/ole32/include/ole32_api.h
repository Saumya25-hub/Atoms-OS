#ifndef BOS_OLE32_API_H
#define BOS_OLE32_API_H

#include "ole32_types.h"

extern const IID IID_IUnknown;
extern const IID IID_IClassFactory;
extern const IID IID_IDataObject;
extern const IID IID_IStream;
extern const IID IID_IStorage;

// System Init
int32_t Ole32Initialize(void);

// COM Runtime
HRESULT CoInitialize(void* pvReserved);
HRESULT CoInitializeEx(void* pvReserved, DWORD dwCoInit);
void    CoUninitialize(void);

// Class Factory & Instantiation
HRESULT CoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, void** ppv);
HRESULT CoRegisterClassObject(REFCLSID rclsid, IUnknown* pUnk, DWORD dwClsContext, DWORD flags, DWORD* lpdwRegister);
HRESULT CoRevokeClassObject(DWORD dwRegister);

// Interface Marshalling
HRESULT CoMarshalInterface(IStream* pStm, REFIID riid, IUnknown* pUnk, DWORD dwDestContext, void* pvDestContext, DWORD mshlflags);
HRESULT CoUnmarshalInterface(IStream* pStm, REFIID riid, void** ppv);

// GUIDs / CLSIDs
HRESULT CoCreateGuid(GUID* pguid);
HRESULT StringFromCLSID(REFCLSID rclsid, char** lplpsz);
HRESULT CLSIDFromString(const char* lpsz, CLSID* pclsid);
HRESULT CoLockObjectExternal(IUnknown* pUnk, BOOL fLock, BOOL fLastUnlockReleases);

// OLE Clipboard & Drag-Drop
HRESULT OleInitialize(void* pvReserved);
void    OleUninitialize(void);
HRESULT OleSetClipboard(IDataObject* pDataObj);
HRESULT OleGetClipboard(IDataObject** ppDataObj);
HRESULT RegisterDragDrop(HANDLE hwnd, IDropTarget* pDropTarget);
HRESULT RevokeDragDrop(HANDLE hwnd);
HRESULT DoDragDrop(IDataObject* pDataObj, IDropSource* pDropSource, DWORD dwOKEffects, DWORD* pdwEffect);

// Structured Storage & Streams
HRESULT CreateStreamOnHGlobal(HANDLE hGlobal, BOOL fDeleteOnRelease, IStream** ppstm);
HRESULT StgCreateStorageEx(const char* pwcsName, DWORD grfMode, DWORD stgfmt, DWORD grfAttrs, void* pStgOptions, void* pSecurityDescriptor, REFIID riid, void** ppObjectOpen);
HRESULT StgOpenStorage(const char* pwcsName, IStorage* pstgPriority, DWORD grfMode, void* snbExclude, DWORD reserved, IStorage** ppstgOpen);

// Diagnostics
void OLE32DumpDiagnostics(void);

#endif // BOS_OLE32_API_H
