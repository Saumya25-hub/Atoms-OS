#include "../include/ole32_api.h"

HRESULT RegisterDragDrop(HANDLE hwnd, IDropTarget* pDropTarget) {
    (void)hwnd; (void)pDropTarget;
    return S_OK;
}

HRESULT RevokeDragDrop(HANDLE hwnd) {
    (void)hwnd;
    return S_OK;
}

HRESULT DoDragDrop(IDataObject* pDataObj, IDropSource* pDropSource, DWORD dwOKEffects, DWORD* pdwEffect) {
    (void)pDataObj; (void)pDropSource; (void)dwOKEffects;
    if (pdwEffect) *pdwEffect = 1; // DROPEFFECT_COPY
    return S_OK;
}
