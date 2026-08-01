#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

HWND CreateDialog(HINSTANCE hInstance, const char* lpTemplateName, HWND hWndParent, WNDPROC lpDialogFunc) {
    (void)hInstance; (void)lpDialogFunc;
    if (!lpTemplateName) return 0;
    return (HWND)BAR_CreateDialog(hWndParent, lpTemplateName, 300, 200);
}

int32_t DialogBox(HINSTANCE hInstance, const char* lpTemplateName, HWND hWndParent, WNDPROC lpDialogFunc) {
    HWND hDlg = CreateDialog(hInstance, lpTemplateName, hWndParent, lpDialogFunc);
    if (!hDlg) return -1;
    BAR_ShowDialog(hDlg);
    return 1;
}

bool EndDialog(HWND hDlg, int32_t nResult) {
    (void)nResult;
    return (BAR_CloseDialog(hDlg) == 0);
}

int32_t MessageBox(HWND hWnd, const char* lpText, const char* lpCaption, uint32_t uType) {
    (void)hWnd; (void)lpText; (void)lpCaption; (void)uType;
    return 1; // IDOK
}
