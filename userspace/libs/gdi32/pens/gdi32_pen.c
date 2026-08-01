#include "../include/gdi32_api.h"

static uint32_t g_pen_counter = 1;

HPEN CreatePen(int32_t iStyle, int32_t cWidth, COLORREF color) {
    (void)iStyle; (void)cWidth; (void)color;
    return (HPEN)(g_pen_counter++);
}
