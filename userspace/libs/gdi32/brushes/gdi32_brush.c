#include "../include/gdi32_api.h"

static uint32_t g_brush_counter = 1;

HBRUSH CreateSolidBrush(COLORREF color) {
    (void)color;
    return (HBRUSH)(g_brush_counter++);
}

HBRUSH CreatePatternBrush(HBITMAP hbm) {
    (void)hbm;
    return (HBRUSH)(g_brush_counter++);
}

HGDIOBJ GetStockObject(int32_t fnObject) {
    return (HGDIOBJ)(100 + fnObject);
}
