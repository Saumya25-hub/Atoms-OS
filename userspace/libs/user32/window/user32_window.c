#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

HWND CreateWindow(const char* lpClassName, const char* lpWindowName, uint32_t dwStyle, int32_t x, int32_t y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, void* lpParam) {
    return CreateWindowEx(0, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

HWND CreateWindowEx(uint32_t dwExStyle, const char* lpClassName, const char* lpWindowName, uint32_t dwStyle, int32_t x, int32_t y, int32_t nWidth, int32_t nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, void* lpParam) {
    (void)dwExStyle; (void)dwStyle; (void)hMenu; (void)hInstance; (void)lpParam;
    if (!lpClassName || !lpWindowName) return 0;
    
    // Delegate Window Creation to BAR Application Runtime
    BARWindowID bar_win = BAR_CreateWindow(1, x, y, nWidth, nHeight, lpWindowName, hWndParent);
    return (HWND)bar_win;
}

bool DestroyWindow(HWND hWnd) {
    if (hWnd == 0) return false;
    return (BAR_DestroyWindow(hWnd) == 0);
}

bool ShowWindow(HWND hWnd, int32_t nCmdShow) {
    (void)nCmdShow;
    if (hWnd == 0) return false;
    return (BAR_ShowWindow(hWnd) == 0);
}

bool HideWindow(HWND hWnd) {
    if (hWnd == 0) return false;
    return (BAR_HideWindow(hWnd) == 0);
}

bool MoveWindow(HWND hWnd, int32_t x, int32_t y, int32_t nWidth, int32_t nHeight, bool bRepaint) {
    (void)x; (void)y; (void)nWidth; (void)nHeight; (void)bRepaint;
    return (hWnd != 0);
}

bool SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int32_t x, int32_t y, int32_t cx, int32_t cy, uint32_t uFlags) {
    (void)hWndInsertAfter; (void)x; (void)y; (void)cx; (void)cy; (void)uFlags;
    return (hWnd != 0);
}

bool UpdateWindow(HWND hWnd) {
    return (hWnd != 0);
}

bool InvalidateRect(HWND hWnd, const RECT* lpRect, bool bErase) {
    (void)lpRect; (void)bErase;
    return (hWnd != 0);
}

bool ValidateRect(HWND hWnd, const RECT* lpRect) {
    (void)lpRect;
    return (hWnd != 0);
}

HDC BeginPaint(HWND hWnd, PAINTSTRUCT* lpPaint) {
    if (!hWnd || !lpPaint) return 0;
    lpPaint->hdc = (HDC)hWnd;
    return lpPaint->hdc;
}

bool EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint) {
    (void)hWnd; (void)lpPaint;
    return true;
}

LRESULT DefWindowProc(HWND hWnd, uint32_t Msg, WPARAM wParam, LPARAM lParam) {
    (void)hWnd; (void)Msg; (void)wParam; (void)lParam;
    return 0;
}
