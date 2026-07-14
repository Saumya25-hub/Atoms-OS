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
    // Disabled: Handled natively by compositor dirty rects
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

volatile uint64_t g_bvcursor_draw_count = 0;

void BVCursor_Draw(int32_t cx, int32_t cy) {
    g_bvcursor_draw_count++;
    extern void BSPE_CursorPresenter_SetCoords(int32_t x, int32_t y);
    BSPE_CursorPresenter_SetCoords(cx, cy);

    /* Phase 5 Backward Compatibility Bridge:
     * Forward authoritative drawing calls to the single compositor overlay renderer.
     */
    extern void* BOVISUAL_Graphics_GetBuffer(void);
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    extern uint32_t BOVISUAL_Graphics_GetPitch(void);
    extern void cursor_engine_render_overlay(const void* fb_ptr);
    
    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = BOVISUAL_Graphics_GetWidth();
    ram_fb.height = BOVISUAL_Graphics_GetHeight();
    ram_fb.pitch = BOVISUAL_Graphics_GetPitch();
    
    cursor_engine_render_overlay(&ram_fb);
}
