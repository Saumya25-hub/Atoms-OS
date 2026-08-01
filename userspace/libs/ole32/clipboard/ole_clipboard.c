#include "../include/ole32_api.h"

static IDataObject* g_ole_clipboard_obj = NULL;

HRESULT OleInitialize(void* pvReserved) {
    return CoInitializeEx(pvReserved, COINIT_APARTMENTTHREADED);
}

void OleUninitialize(void) {
    CoUninitialize();
}

HRESULT OleSetClipboard(IDataObject* pDataObj) {
    g_ole_clipboard_obj = pDataObj;
    return S_OK;
}

HRESULT OleGetClipboard(IDataObject** ppDataObj) {
    if (!ppDataObj) return E_POINTER;
    *ppDataObj = g_ole_clipboard_obj;
    return S_OK;
}
