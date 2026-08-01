#include "../include/gdi32_api.h"

static COLORREF g_cur_text_color = RGB(0, 0, 0);
static COLORREF g_cur_bk_color = RGB(255, 255, 255);

bool TextOut(HDC hdc, int32_t x, int32_t y, const char* lpString, int32_t c) {
    (void)hdc; (void)x; (void)y; (void)lpString; (void)c;
    return (hdc != 0);
}

int32_t DrawText(HDC hdc, const char* lpchText, int32_t cchText, RECT* lprc, uint32_t format) {
    (void)hdc; (void)lpchText; (void)cchText; (void)lprc; (void)format;
    return 16; // Height in pixels
}

bool ExtTextOut(HDC hdc, int32_t x, int32_t y, uint32_t options, const RECT* lprect, const char* lpString, uint32_t c, const int32_t* lpDx) {
    (void)hdc; (void)x; (void)y; (void)options; (void)lprect; (void)lpString; (void)c; (void)lpDx;
    return (hdc != 0);
}

COLORREF SetTextColor(HDC hdc, COLORREF color) {
    (void)hdc;
    COLORREF old = g_cur_text_color;
    g_cur_text_color = color;
    return old;
}

COLORREF SetBkColor(HDC hdc, COLORREF color) {
    (void)hdc;
    COLORREF old = g_cur_bk_color;
    g_cur_bk_color = color;
    return old;
}
