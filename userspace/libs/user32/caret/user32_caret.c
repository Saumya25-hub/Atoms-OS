#include "../include/user32_api.h"

static bool g_caret_visible = false;

bool CreateCaret(HWND hWnd, HBRUSH hBitmap, int32_t nWidth, int32_t nHeight) {
    (void)hWnd; (void)hBitmap; (void)nWidth; (void)nHeight;
    g_caret_visible = true;
    return true;
}

bool ShowCaret(HWND hWnd) {
    (void)hWnd;
    g_caret_visible = true;
    return true;
}

bool HideCaret(HWND hWnd) {
    (void)hWnd;
    g_caret_visible = false;
    return true;
}
