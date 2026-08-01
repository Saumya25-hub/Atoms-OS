#include "../include/gdi32_api.h"

bool MoveToEx(HDC hdc, int32_t x, int32_t y, POINT* lppt) {
    (void)hdc;
    if (lppt) { lppt->x = 0; lppt->y = 0; }
    return (hdc != 0);
}

bool LineTo(HDC hdc, int32_t x, int32_t y) {
    (void)hdc; (void)x; (void)y;
    return (hdc != 0);
}

COLORREF SetPixel(HDC hdc, int32_t x, int32_t y, COLORREF color) {
    (void)hdc; (void)x; (void)y;
    return color;
}

COLORREF GetPixel(HDC hdc, int32_t x, int32_t y) {
    (void)hdc; (void)x; (void)y;
    return RGB(0, 0, 0);
}
