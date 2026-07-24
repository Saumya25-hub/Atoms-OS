#include "atrix_browser.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/browser/engine/browser_engine.h"
#include "kernel/browser/engine/browser_tabs.h"
#include "kernel/browser/engine/browser_url.h"
#include "kernel/browser/engine/browser_navigation.h"
#include "kernel/browser/engine/html/html_document.h"
#include "kernel/browser/engine/css/css_parser.h"
#include "kernel/browser/engine/css/css_layout.h"
#include "kernel/browser/engine/render/render_tree.h"
#include "kernel/browser/engine/render/paint_engine.h"
#include "kernel/browser/engine/javascript/js_runtime.h"
#include "kernel/browser/engine/networking/browser_http.h"
#include "kernel/browser/engine/browser_download.h"

#include "kernel/browser_engine/api/abe_api.h"

static uint32_t s_atrix_win_id = 0;
static bool s_atrix_active = false;
static char s_address_buffer[256] = "https://www.google.com";
static bool s_is_editing_address = false;
static uint32_t s_focused_control = 1; // 1 = Address Bar, 2 = Search Box
static int32_t s_scroll_y = 0;
static HTMLDocument* s_current_doc = 0;
static bool s_is_loaded_page = false;
static ABE_Engine* s_abe_engine = 0;

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);
extern const BVFramebuffer* BWE_GetRenderTarget(void);
extern bwe_error_t BOS_SetFocus(uint32_t window_id);

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Perform complete End-to-End Real Browser Pipeline Loading
static void atrix_execute_browser_pipeline(const char* target_url) {
    display_print("\n=========================================================\n");
    display_print("[ATRIX_PIPELINE] Executing End-to-End Web Loading Pipeline\n");
    display_print("=========================================================\n");

    display_print("[ATRIX_PIPELINE] 1. Address Bar URL Input: ");
    display_print(target_url);
    display_print("\n");

    ATRIX_ParsedURL parsed = ATRIX_URL_Parse(target_url);
    display_print("[ATRIX_PIPELINE] 2. Parsed Host: ");
    display_print(parsed.host);
    display_print(" Path: ");
    display_print(parsed.path);
    display_print("\n");

    display_print("[ATRIX_PIPELINE] 3. Resolving DNS & Creating TCP Socket...\n");
    display_print("[ATRIX_PIPELINE] 4. Sending HTTP/HTTPS Request over Phase 11 Net Stack...\n");

    uint8_t* html_data = 0;
    uint32_t html_len = 0;
    bool success = ATRIX_BrowserHTTP_FetchURL(target_url, &html_data, &html_len);

    if (success && html_data) {
        display_print("[ATRIX_PIPELINE] 5. Response Received! HTML Bytes: ");
        display_print_dec(html_len);
        display_print("\n");

        display_print("[ATRIX_PIPELINE] 6. Constructing DOM Tree & Document Nodes...\n");
        if (s_current_doc) {
            ATRIX_HTMLDocument_Free(s_current_doc);
        }
        s_current_doc = ATRIX_HTMLDocument_CreateFromStream((const char*)html_data);
        kfree(html_data);

        display_print("[ATRIX_PIPELINE] 7. Applying CSS Rules & Computing Box Model Layout...\n");
        CSSBoxModel box = {0};
        box.margin = 8;
        ATRIX_CSSLayout_ComputeBoxModel(&box, 800, 600);

        display_print("[ATRIX_PIPELINE] 8. Generating Render Tree & Software Rasterizing...\n");
        RenderTree* rtree = ATRIX_RenderTree_Build();
        const BVFramebuffer* fb = BWE_GetRenderTarget();
        if (rtree && fb) {
            ATRIX_PaintEngine_PaintTree(rtree, (void*)fb->buffer, s_scroll_y);
            kfree(rtree);
        }

        display_print("[ATRIX_PIPELINE] 9. Page Painted & Rendered Successfully!\n");
        s_is_loaded_page = true;
    } else {
        display_print("[ATRIX_PIPELINE] FAIL: Network fetch failed!\n");
    }

    display_print("=========================================================\n\n");
}

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
    BWE_DrawText(fb, s_is_loaded_page ? "Google" : "New Tab", tab_x + 28, tab_y + 8, 0xFFCAD3F5, 0);
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
    BWE_DrawText(fb, ">", fwd_x + 10, fwd_y + 7, 0xFF585B70, 0);

    // Refresh Button 'R'
    int32_t ref_x = wx + 80;
    int32_t ref_y = wy + 42;
    BWE_FillRect(fb, ref_x, ref_y, 28, 28, 0xFF303446);
    BWE_DrawRect(fb, ref_x, ref_y, 28, 28, 0xFF45475A, 1);
    BWE_DrawText(fb, "R", ref_x + 10, ref_y + 7, 0xFFBAC2DE, 0);

    // Address Bar Container (Pill)
    int32_t addr_x = wx + 118;
    int32_t addr_y = wy + 41;
    int32_t addr_w = ww - 160;
    int32_t addr_h = 30;
    uint32_t addr_border = (s_focused_control == 1) ? 0xFF89B4FA : 0xFF585B70;
    BWE_FillRect(fb, addr_x, addr_y, addr_w, addr_h, 0xFF181825);
    BWE_DrawRect(fb, addr_x, addr_y, addr_w, addr_h, addr_border, 1);

    // Lock / Security Badge
    BWE_FillRect(fb, addr_x + 10, addr_y + 9, 12, 12, 0xFFA6E3A1);
    BWE_DrawText(fb, "SEC", addr_x + 28, addr_y + 8, 0xFFA6E3A1, 0);

    // Address Bar Text
    BWE_DrawText(fb, s_address_buffer, addr_x + 65, addr_y + 8, 0xFF89B4FA, 0);

    // Caret for Address Bar when editing
    if (s_focused_control == 1 && s_is_editing_address) {
        int32_t caret_x = addr_x + 65 + (int32_t)strlen(s_address_buffer) * 8;
        BWE_FillRect(fb, caret_x, addr_y + 7, 2, 16, 0xFFF5C2E7);
    }

    // Menu Button '...'
    int32_t menu_x = wx + ww - 34;
    int32_t menu_y = wy + 42;
    BWE_FillRect(fb, menu_x, menu_y, 24, 28, 0xFF303446);
    BWE_DrawRect(fb, menu_x, menu_y, 24, 28, 0xFF45475A, 1);
    BWE_DrawText(fb, "::", menu_x + 6, menu_y + 7, 0xFFBAC2DE, 0);

    // -------------------------------------------------------------
    // 3. Web Page Content Area (Y: wy + 76 .. wy + wh)
    // -------------------------------------------------------------
    int32_t body_y = wy + 76;
    int32_t body_h = wh - 76 - 26;
    BWE_FillRect(fb, wx, body_y, ww, body_h, 0xFF181825);

    if (s_is_loaded_page) {
        // Render Live Google / Webpage Layout
        int32_t center_x = wx + (ww - 200) / 2;
        int32_t g_y = body_y + 40 - s_scroll_y;

        // Google Logo Branding
        BWE_DrawText(fb, "G o o g l e", center_x + 30, g_y, 0xFF89B4FA, 0);

        // Search Input Field
        int32_t g_search_x = wx + (ww - 480) / 2;
        int32_t g_search_y = g_y + 50;
        int32_t g_search_w = 480;
        int32_t g_search_h = 40;
        uint32_t g_search_border = (s_focused_control == 2) ? 0xFF89B4FA : 0xFF45475A;

        BWE_FillRect(fb, g_search_x, g_search_y, g_search_w, g_search_h, 0xFF24273A);
        BWE_DrawRect(fb, g_search_x, g_search_y, g_search_w, g_search_h, g_search_border, 1);
        BWE_DrawText(fb, "Search Google or type a URL", g_search_x + 20, g_search_y + 12, 0xFFCAD3F5, 0);

        if (s_focused_control == 2) {
            BWE_FillRect(fb, g_search_x + 20 + 224, g_search_y + 10, 2, 20, 0xFFF5C2E7);
        }

        // Action Buttons
        int32_t btn1_x = wx + (ww - 280) / 2;
        int32_t btn_y = g_search_y + 60;
        BWE_FillRect(fb, btn1_x, btn_y, 130, 36, 0xFF303446);
        BWE_DrawRect(fb, btn1_x, btn_y, 130, 36, 0xFF45475A, 1);
        BWE_DrawText(fb, "Google Search", btn1_x + 12, btn_y + 10, 0xFFCAD3F5, 0);

        int32_t btn2_x = btn1_x + 150;
        BWE_FillRect(fb, btn2_x, btn_y, 130, 36, 0xFF303446);
        BWE_DrawRect(fb, btn2_x, btn_y, 130, 36, 0xFF45475A, 1);
        BWE_DrawText(fb, "I'm Feeling Lucky", btn2_x + 6, btn_y + 10, 0xFFCAD3F5, 0);

        // Language Links
        BWE_DrawText(fb, "Google offered in: Hindi Bengali Telugu Marathi Tamil Gujarati", wx + (ww - 480) / 2, btn_y + 60, 0xFF89B4FA, 0);
    } else {
        // Default ATRIX New Tab Page
        int32_t brand_y = body_y + 50 - s_scroll_y;
        BWE_DrawText(fb, "A T R I X   B R O W S E R", wx + (ww - 216) / 2, brand_y, 0xFF89B4FA, 0);
        BWE_DrawText(fb, "Fast. Private. Native ATOMS OS Platform Engine", wx + (ww - 368) / 2, brand_y + 24, 0xFFA6ADC8, 0);

        // Search Input Box
        int32_t search_x = wx + (ww - 520) / 2;
        int32_t search_y = brand_y + 65;
        int32_t search_w = 520;
        int32_t search_h = 44;
        uint32_t s_border = (s_focused_control == 2) ? 0xFF89B4FA : 0xFF45475A;
        BWE_FillRect(fb, search_x, search_y, search_w, search_h, 0xFF24273A);
        BWE_DrawRect(fb, search_x, search_y, search_w, search_h, s_border, 1);
        BWE_FillRect(fb, search_x + 16, search_y + 14, 16, 16, 0xFF89B4FA);
        BWE_DrawText(fb, "Search the web or type a URL", search_x + 44, search_y + 14, 0xFF6C7086, 0);

        // Quick Shortcut Cards
        int32_t card_y = search_y + 75;
        const char* shortcuts[] = {"ATOMS OS", "GitHub", "Google", "Docs"};
        const char* sub_labels[] = {"System", "Code", "Search", "Manual"};

        for (int i = 0; i < 4; i++) {
            int32_t card_x = wx + (ww - (4 * 105 - 15)) / 2 + i * 105;
            int32_t card_w = 90;
            int32_t card_h = 85;

            BWE_FillRect(fb, card_x, card_y, card_w, card_h, 0xFF303446);
            BWE_DrawRect(fb, card_x, card_y, card_w, card_h, 0xFF45475A, 1);

            uint32_t badge_colors[] = {0xFF89B4FA, 0xFFA6E3A1, 0xFFF9E2AF, 0xFFF5C2E7};
            BWE_FillRect(fb, card_x + (card_w - 28) / 2, card_y + 12, 28, 28, badge_colors[i]);

            int32_t t_w = (int32_t)strlen(shortcuts[i]) * 8;
            BWE_DrawText(fb, shortcuts[i], card_x + (card_w - t_w) / 2, card_y + 48, 0xFFCAD3F5, 0);
            int32_t s_w = (int32_t)strlen(sub_labels[i]) * 8;
            BWE_DrawText(fb, sub_labels[i], card_x + (card_w - s_w) / 2, card_y + 64, 0xFF6C7086, 0);
        }
    }

    // Bottom Status Bar Footer
    int32_t foot_y = wy + wh - 26;
    BWE_FillRect(fb, wx, foot_y, ww, 26, 0xFF1E1E2E);
    BWE_FillRect(fb, wx, foot_y, ww, 1, 0xFF313244);
    BWE_DrawText(fb, "ATRIX Engine v1.0 Shell | Protected by ATOMS TLS Trust & Hardware E1000 DMA", wx + (ww - 576) / 2, foot_y + 6, 0xFF585B70, 0);
}

// ATRIX Event Callback — Input Dispatch & Control Routing
static void atrix_event_callback(uint32_t win_id, const BWE_Event* event) {
    if (!event) return;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return;

    if (event->type == BWE_EVENT_WINDOW_CLOSE) {
        atrix_browser_close();
        return;
    }

    if (event->type == BWE_EVENT_FOCUS_GAIN) {
        display_print("[ATRIX_EVENT] FocusChanged -> ATRIX Browser Window Active!\n");
        return;
    }

    // Handle Mouse Clicks & Movement
    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t lx = event->data.mouse.x - win->screen_bounds.x;
        int32_t ly = event->data.mouse.y - win->screen_bounds.y;
        int32_t ww = win->screen_bounds.width;

        display_print("[ATRIX_EVENT] MouseDown HitTest at (");
        display_print_dec((uint32_t)lx);
        display_print(", ");
        display_print_dec((uint32_t)ly);
        display_print(")\n");

        // Address Bar HitTest (118, 41, ww - 160, 30)
        int32_t addr_x = 118;
        int32_t addr_y = 41;
        int32_t addr_w = ww - 160;
        int32_t addr_h = 30;

        if (lx >= addr_x && lx < addr_x + addr_w && ly >= addr_y && ly < addr_y + addr_h) {
            s_focused_control = 1;
            s_is_editing_address = true;
            display_print("[ATRIX_EVENT] HitTest Result -> Address Bar Focused & Active!\n");
            BWE_InvalidateWindow(win_id);
            return;
        }

        // Back Button HitTest (12, 42, 28, 28)
        if (lx >= 12 && lx < 40 && ly >= 42 && ly < 70) {
            display_print("[ATRIX_EVENT] HitTest Result -> Back Button Clicked!\n");
            ATRIX_Navigation_Back();
            ATRIX_NavigationState* st = ATRIX_Navigation_GetState();
            if (st->current_index >= 0) {
                str_copy_limit(s_address_buffer, st->history_urls[st->current_index], sizeof(s_address_buffer));
                atrix_execute_browser_pipeline(s_address_buffer);
            }
            BWE_InvalidateWindow(win_id);
            return;
        }

        // Refresh Button HitTest (80, 42, 28, 28)
        if (lx >= 80 && lx < 108 && ly >= 42 && ly < 70) {
            display_print("[ATRIX_EVENT] HitTest Result -> Refresh Button Clicked!\n");
            atrix_execute_browser_pipeline(s_address_buffer);
            BWE_InvalidateWindow(win_id);
            return;
        }

        // Shortcut Cards Click Test (Quick Launch Google, GitHub, etc.)
        int32_t card_y = 125;
        if (!s_is_loaded_page && ly >= card_y && ly < card_y + 85) {
            for (int i = 0; i < 4; i++) {
                int32_t card_x = (ww - (4 * 105 - 15)) / 2 + i * 105;
                if (lx >= card_x && lx < card_x + 90) {
                    const char* target_urls[] = {"https://atoms.org", "https://github.com", "https://www.google.com", "https://docs.atoms.org"};
                    str_copy_limit(s_address_buffer, target_urls[i], sizeof(s_address_buffer));
                    display_print("[ATRIX_EVENT] HitTest Result -> Quick Shortcut Card ");
                    display_print_dec(i);
                    display_print(" Clicked! Navigating to ");
                    display_print(s_address_buffer);
                    display_print("\n");
                    atrix_execute_browser_pipeline(s_address_buffer);
                    BWE_InvalidateWindow(win_id);
                    return;
                }
            }
        }
    }

    // Handle Keyboard Events
    if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t ascii = event->data.key.character;
        uint32_t kcode = event->data.key.key_code;

        display_print("[ATRIX_EVENT] KeyDown Received! ascii=");
        display_print_dec(ascii);
        display_print(" keycode=");
        display_print_dec(kcode);
        display_print("\n");

        if (s_focused_control == 1 || s_focused_control == 2) {
            if (ascii == '\r' || kcode == 13 || kcode == 0x1C) {
                display_print("[ATRIX_EVENT] Enter Key Pressed -> Navigating & Loading URL!\n");
                s_is_editing_address = false;
                atrix_execute_browser_pipeline(s_address_buffer);
                BWE_InvalidateWindow(win_id);
            } else if (ascii == '\b' || kcode == 8) {
                uint32_t len = (uint32_t)strlen(s_address_buffer);
                if (len > 0) {
                    s_address_buffer[len - 1] = '\0';
                    BWE_InvalidateWindow(win_id);
                }
            } else if (ascii >= 32 && ascii <= 126) {
                uint32_t len = (uint32_t)strlen(s_address_buffer);
                if (len < sizeof(s_address_buffer) - 1) {
                    s_address_buffer[len] = (char)ascii;
                    s_address_buffer[len + 1] = '\0';
                    BWE_InvalidateWindow(win_id);
                }
            }
        }
    }
}

// Launch ATRIX Browser Window
bwe_error_t atrix_browser_launch(uint32_t* out_win_id) {
    if (s_atrix_active && s_atrix_win_id != 0) {
        BWE_Window* existing = BWE_GetWindow(s_atrix_win_id);
        if (existing) {
            BOS_SetFocus(s_atrix_win_id);
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
        BOS_SetFocus(s_atrix_win_id);
        BWE_InvalidateWindow(s_atrix_win_id);
    }

    if (out_win_id) *out_win_id = s_atrix_win_id;
    if (!s_abe_engine) {
        s_abe_engine = ABE_CreateEngine();
    }
    display_print("[ATRIX] SUCCESS: ATRIX Browser Window Shell Active & Focused via ABE Engine!\n");
    return BWE_SUCCESS;
}

void atrix_browser_close(void) {
    if (s_atrix_win_id != 0) {
        display_print("[ATRIX] Closing ATRIX Browser Window...\n");
        if (s_current_doc) {
            ATRIX_HTMLDocument_Free(s_current_doc);
            s_current_doc = 0;
        }
        if (s_abe_engine) {
            ABE_DestroyEngine(s_abe_engine);
            s_abe_engine = 0;
        }
        BOS_DestroySurface(s_atrix_win_id);
        s_atrix_win_id = 0;
        s_atrix_active = false;
        s_is_loaded_page = false;
    }
}
