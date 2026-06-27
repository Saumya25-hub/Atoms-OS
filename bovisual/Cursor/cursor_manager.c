#include "../Include/cursor_manager.h"
#include "../Include/graphics.h"
#include "../Include/drawing.h"

#define CURSOR_W 12
#define CURSOR_H 18

static BOVISUAL_Color cursor_bg[CURSOR_H][CURSOR_W];
static int32_t last_cursor_x = -1;
static int32_t last_cursor_y = -1;
static bool cursor_saved = false;

static uint32_t max_x = 0;
static uint32_t max_y = 0;

void BVCursor_Init(uint32_t screen_width, uint32_t screen_height) {
    max_x = screen_width;
    max_y = screen_height;
    cursor_saved = false;
}

void BVCursor_RestoreBG(void) {
    if (!cursor_saved) return;
    
    for (int y = 0; y < CURSOR_H; y++) {
        for (int x = 0; x < CURSOR_W; x++) {
            int32_t draw_x = last_cursor_x + x;
            int32_t draw_y = last_cursor_y + y;
            if (draw_x >= 0 && draw_x < (int32_t)max_x && draw_y >= 0 && draw_y < (int32_t)max_y) {
                BOVISUAL_Graphics_PutPixel(draw_x, draw_y, cursor_bg[y][x]);
            }
        }
    }
    cursor_saved = false;
}

static void SafeDrawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, BOVISUAL_Color color) {
    // Basic clamping check before forwarding to BOVISUAL_Draw_Line 
    // to ensure the full dimensions are bounded by the Cursor Manager.
    if (x0 >= (int32_t)max_x) x0 = max_x - 1;
    if (y0 >= (int32_t)max_y) y0 = max_y - 1;
    if (x1 >= (int32_t)max_x) x1 = max_x - 1;
    if (y1 >= (int32_t)max_y) y1 = max_y - 1;
    
    BOVISUAL_Draw_Line(x0, y0, x1, y1, color);
}

void BVCursor_Draw(int32_t cx, int32_t cy) {
    // 1. Restore old background
    BVCursor_RestoreBG();

    // 2. Save new background (respecting bounds)
    for (int y = 0; y < CURSOR_H; y++) {
        for (int x = 0; x < CURSOR_W; x++) {
            int32_t save_x = cx + x;
            int32_t save_y = cy + y;
            
            if (save_x >= 0 && save_x < (int32_t)max_x && save_y >= 0 && save_y < (int32_t)max_y) {
                cursor_bg[y][x] = BOVISUAL_Graphics_ReadPixel(save_x, save_y);
            } else {
                cursor_bg[y][x] = 0;
            }
        }
    }
    last_cursor_x = cx;
    last_cursor_y = cy;
    cursor_saved = true;

    // 3. Draw Cursor (clipping enforced via SafeDrawLine)
    BOVISUAL_Color cursor_color = 0xFFFFFFFF; // White
    BOVISUAL_Color outline_color = 0xFF000000; // Black
    
    SafeDrawLine(cx, cy, cx, cy + 15, outline_color);
    SafeDrawLine(cx, cy, cx + 10, cy + 10, outline_color);
    SafeDrawLine(cx, cy + 15, cx + 3, cy + 11, outline_color);
    SafeDrawLine(cx + 3, cy + 11, cx + 10, cy + 10, outline_color);
    
    for (int i = 1; i < 10; i++) {
        SafeDrawLine(cx + 1, cy + i, cx + i / 2, cy + i, cursor_color);
    }
}
