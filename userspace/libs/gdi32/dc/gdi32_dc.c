#include "../include/gdi32_api.h"

typedef struct {
    HDC       hdc_id;
    COLORREF  text_color;
    COLORREF  bk_color;
    HPEN      pen;
    HBRUSH    brush;
    HFONT     font;
    HBITMAP   bitmap;
    POINT     cur_pos;
    bool      active;
} GDI32DCEntry;

static GDI32DCEntry g_dc_table[64];
static uint32_t g_dc_counter = 1;

HDC CreateCompatibleDC(HDC hdc) {
    (void)hdc;
    for (int i = 0; i < 64; i++) {
        if (!g_dc_table[i].active) {
            g_dc_table[i].hdc_id = g_dc_counter++;
            g_dc_table[i].text_color = RGB(0, 0, 0);
            g_dc_table[i].bk_color = RGB(255, 255, 255);
            g_dc_table[i].pen = 0;
            g_dc_table[i].brush = 0;
            g_dc_table[i].font = 0;
            g_dc_table[i].bitmap = 0;
            g_dc_table[i].cur_pos.x = 0;
            g_dc_table[i].cur_pos.y = 0;
            g_dc_table[i].active = true;
            return g_dc_table[i].hdc_id;
        }
    }
    return 0;
}

bool DeleteDC(HDC hdc) {
    if (hdc == 0) return false;
    for (int i = 0; i < 64; i++) {
        if (g_dc_table[i].active && g_dc_table[i].hdc_id == hdc) {
            g_dc_table[i].active = false;
            return true;
        }
    }
    return false;
}

int32_t SaveDC(HDC hdc) {
    return (hdc != 0) ? 1 : 0;
}

bool RestoreDC(HDC hdc, int32_t nSavedDC) {
    (void)nSavedDC;
    return (hdc != 0);
}

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ hso) {
    if (hdc == 0) return 0;
    for (int i = 0; i < 64; i++) {
        if (g_dc_table[i].active && g_dc_table[i].hdc_id == hdc) {
            HGDIOBJ old = g_dc_table[i].pen;
            g_dc_table[i].pen = hso;
            return old;
        }
    }
    return hso;
}

bool DeleteObject(HGDIOBJ hObject) {
    return (hObject != 0);
}
