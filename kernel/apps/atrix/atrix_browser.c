#include "atrix_browser.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/engine/horse_engine.h"

static uint32_t s_atrix_win_id = 0;
static bool s_atrix_active = false;
static char s_address_buffer[256] = "atrix://newtab";

extern void display_print(const char* s);
extern const BVFramebuffer* BWE_GetRenderTarget(void);

// ATRIX Render Callback
static void atrix_render_callback(BWE_Window* self) {
    if (!self) return;
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    BWE_Rect b = self->screen_bounds;
    int32_t wx = b.x;
    int32_t wy = b.y;
    int32_t ww = b.width;
    int32_t wh = b.height;

    // -------------------------------------------------------------
    // 1. Top Tab Bar Container (Y: wy .. wy + 36)
    // -------------------------------------------------------------
    BWE_FillRect(fb, wx, wy, ww, 36, 0xFF1E1E2E);

    // Active Tab ("New Tab")
    int32_t tab_x = wx + 8;
    int32_t tab_y = wy + 6;
    int32_t tab_w = 180;
    int32_t tab_h = 30;
    BWE_FillRect(fb, tab_x, tab_y, tab_w, tab_h, 0xFF303446);
    BWE_DrawRect(fb, tab_x, tab_y, tab_w, tab_h, 0xFF45475A, 1);

    // Tab Favicon (Cyan dot)
    BWE_FillRect(fb, tab_x + 10, tab_y + 10, 10, 10, 0xFF89DCEB);
    // Tab Title
    BWE_DrawText(fb, "New Tab", tab_x + 28, tab_y + 8, 0xFFCAD3F5, 0);
    // Tab Close Button 'x'
    BWE_DrawText(fb, "x", tab_x + 160, tab_y + 8, 0xFFA6ADC8, 0);

    // New Tab Button '+'
    int32_t btn_plus_x = tab_x + tab_w + 8;
    int32_t btn_plus_y = wy + 7;
    BWE_FillRect(fb, btn_plus_x, btn_plus_y, 28, 28, 0xFF24273A);
    BWE_DrawRect(fb, btn_plus_x, btn_plus_y, 28, 28, 0xFF45475A, 1);
    BWE_DrawText(fb, "+", btn_plus_x + 10, btn_plus_y + 7, 0xFFBAC2DE, 0);

    // -------------------------------------------------------------
    // 2. Navigation & Address Toolbar Container (Y: wy + 36 .. wy + 76)
    // -------------------------------------------------------------
    BWE_FillRect(fb, wx, wy + 36, ww, 40, 0xFF24273A);
    BWE_FillRect(fb, wx, wy + 75, ww, 1, 0xFF363A4F); // Divider line

    // Back Button '<'
    int32_t back_x = wx + 12;
    int32_t back_y = wy + 42;
    BWE_FillRect(fb, back_x, back_y, 28, 28, 0xFF303446);
    BWE_DrawRect(fb, back_x, back_y, 28, 28, 0xFF45475A, 1);
    BWE_DrawText(fb, "<", back_x + 10, back_y + 7, 0xFFBAC2DE, 0);

    // Forward Button '>'
    int32_t fwd_x = wx + 46;
    int32_t fwd_y = wy + 42;
    BWE_FillRect(fb, fwd_x, fwd_y, 28, 28, 0xFF303446);
    BWE_DrawRect(fb, fwd_x, fwd_y, 28, 28, 0xFF45475A, 1);
    BWE_DrawText(fb, ">", fwd_x + 10, fwd_y + 7, 0xFF585B70, 0); // Disabled tone

    // Refresh Button 'R'
    int32_t ref_x = wx + 80;
    int32_t ref_y = wy + 42;
    BWE_FillRect(fb, ref_x, ref_y, 28, 28, 0xFF303446);
    BWE_DrawRect(fb, ref_x, ref_y, 28, 28, 0xFF45475A, 1);
    BWE_DrawText(fb, "R", ref_x + 10, ref_y + 7, 0xFFBAC2DE, 0);

    // Address Bar Container (Rounded Pill)
    int32_t addr_x = wx + 118;
    int32_t addr_y = wy + 41;
    int32_t addr_w = ww - 160;
    int32_t addr_h = 30;
    BWE_FillRect(fb, addr_x, addr_y, addr_w, addr_h, 0xFF181825);
    BWE_DrawRect(fb, addr_x, addr_y, addr_w, addr_h, 0xFF585B70, 1);

    // Lock / Security Badge
    BWE_FillRect(fb, addr_x + 10, addr_y + 9, 12, 12, 0xFFA6E3A1); // Green Lock badge
    BWE_DrawText(fb, "SEC", addr_x + 28, addr_y + 8, 0xFFA6E3A1, 0);

    // Address Bar Text
    BWE_DrawText(fb, s_address_buffer, addr_x + 65, addr_y + 8, 0xFF89B4FA, 0);

    // Menu Button '...'
    int32_t menu_x = wx + ww - 34;
    int32_t menu_y = wy + 42;
    BWE_FillRect(fb, menu_x, menu_y, 24, 28, 0xFF303446);
    BWE_DrawRect(fb, menu_x, menu_y, 24, 28, 0xFF45475A, 1);
    BWE_DrawText(fb, "::", menu_x + 6, menu_y + 7, 0xFFBAC2DE, 0);

    // -------------------------------------------------------------
    // 3. New Tab Content Area (Y: wy + 76 .. wy + wh)
    // -------------------------------------------------------------
    int32_t body_y = wy + 76;
    int32_t body_h = wh - 76;
    BWE_FillRect(fb, wx, body_y, ww, body_h, 0xFF181825);

    // Branding Header
    int32_t brand_y = body_y + 80;
    BWE_DrawText(fb, "A T R I X   B R O W S E R", wx + (ww - 216) / 2, brand_y, 0xFF89B4FA, 0);
    BWE_DrawText(fb, "Fast. Private. Native ATOMS OS Platform Engine", wx + (ww - 368) / 2, brand_y + 24, 0xFFA6ADC8, 0);

    // Search Input Box
    int32_t search_x = wx + (ww - 520) / 2;
    int32_t search_y = brand_y + 70;
    int32_t search_w = 520;
    int32_t search_h = 44;
    BWE_FillRect(fb, search_x, search_y, search_w, search_h, 0xFF24273A);
    BWE_DrawRect(fb, search_x, search_y, search_w, search_h, 0xFF45475A, 1);

    BWE_FillRect(fb, search_x + 16, search_y + 14, 16, 16, 0xFF89B4FA); // Search icon box
    BWE_DrawText(fb, "Search the web or type a URL", search_x + 44, search_y + 14, 0xFF6C7086, 0);

    // Quick Shortcut Cards
    int32_t card_y = search_y + 80;
    const char* shortcuts[] = {"ATOMS OS", "GitHub", "Google", "Docs"};
    const char* sub_labels[] = {"System", "Code", "Search", "Manual"};

    for (int i = 0; i < 4; i++) {
        int32_t card_x = wx + (ww - (4 * 105 - 15)) / 2 + i * 105;
        int32_t card_w = 90;
        int32_t card_h = 85;

        BWE_FillRect(fb, card_x, card_y, card_w, card_h, 0xFF303446);
        BWE_DrawRect(fb, card_x, card_y, card_w, card_h, 0xFF45475A, 1);

        // Icon badge inside card
        uint32_t badge_colors[] = {0xFF89B4FA, 0xFFA6E3A1, 0xFFF9E2AF, 0xFFF5C2E7};
        BWE_FillRect(fb, card_x + (card_w - 28) / 2, card_y + 12, 28, 28, badge_colors[i]);

        // Labels
        int32_t t_w = strlen(shortcuts[i]) * 8;
        BWE_DrawText(fb, shortcuts[i], card_x + (card_w - t_w) / 2, card_y + 48, 0xFFCAD3F5, 0);
        int32_t s_w = strlen(sub_labels[i]) * 8;
        BWE_DrawText(fb, sub_labels[i], card_x + (card_w - s_w) / 2, card_y + 64, 0xFF6C7086, 0);
    }

    // Bottom Status Bar Footer
    int32_t foot_y = wy + wh - 26;
    BWE_FillRect(fb, wx, foot_y, ww, 26, 0xFF1E1E2E);
    BWE_FillRect(fb, wx, foot_y, ww, 1, 0xFF313244);
    BWE_DrawText(fb, "ATRIX Engine v1.0 Shell | Protected by ATOMS TLS Trust & Hardware E1000 DMA", wx + (ww - 576) / 2, foot_y + 6, 0xFF585B70, 0);
}

// ATRIX Event Callback
static void atrix_event_callback(uint32_t win_id, const BWE_Event* event) {
    if (!event) return;
    if (event->type == BWE_EVENT_WINDOW_CLOSE) {
        atrix_browser_close();
    }
}

// Launch ATRIX Browser Window
bwe_error_t atrix_browser_launch(uint32_t* out_win_id) {
    if (s_atrix_active && s_atrix_win_id != 0) {
        BWE_Window* existing = BWE_GetWindow(s_atrix_win_id);
        if (existing) {
            BWE_InvalidateWindow(s_atrix_win_id);
            if (out_win_id) *out_win_id = s_atrix_win_id;
            return BWE_SUCCESS;
        }
    }

    display_print("[ATRIX] Launching ATRIX Browser Native Window Shell...\n");

    int32_t win_w = 960;
    int32_t win_h = 600;
    int32_t win_x = 480;
    int32_t win_y = 200;

    bwe_error_t err = BOS_CreateSurface(BWE_DESKTOP_ID, win_x, win_y, win_w, win_h,
                                        BWE_WINDOW_CHILD | BWE_WINDOW_MOVABLE,
                                        &s_atrix_win_id);
    if (err != BWE_SUCCESS) {
        display_print("[ATRIX] FAIL: Failed to create ATRIX window surface!\n");
        return err;
    }

    BWE_Window* win = BWE_GetWindow(s_atrix_win_id);
    if (win) {
        win->type = BWE_TYPE_WINDOW;
        win->on_render = atrix_render_callback;
        win->on_event = atrix_event_callback;
        win->user_data = (void*)APP_ID_ATRIX;
        s_atrix_active = true;
        BWE_InvalidateWindow(s_atrix_win_id);
    }

    if (out_win_id) *out_win_id = s_atrix_win_id;
    display_print("[ATRIX] SUCCESS: ATRIX Browser Window Shell Active!\n");
    return BWE_SUCCESS;
}

void atrix_browser_close(void) {
    if (s_atrix_win_id != 0) {
        display_print("[ATRIX] Closing ATRIX Browser Window...\n");
        BOS_DestroySurface(s_atrix_win_id);
        s_atrix_win_id = 0;
        s_atrix_active = false;
    }
}
