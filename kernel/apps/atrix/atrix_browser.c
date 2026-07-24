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

#include "sdk/include/abe/abe.h"

// Global State
static uint32_t s_atrix_win_id = 0;
static bool s_atrix_active = false;
static char s_address_buffer[256] = "chrome://newtab";
static char s_search_buffer[256] = "";
static uint32_t s_focused_control = 2; // 1 = Address Bar, 2 = Main Search / Content
static int32_t s_scroll_y = 0;
static HTMLDocument* s_current_doc = 0;
static bool s_is_loaded_page = true;
static int32_t s_active_tab_index = 0;
static uint32_t s_settings_selected_category = 0; // 0 = Get started, 1 = Appearance, 2 = Shields, 3 = Privacy

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);
extern const BVFramebuffer* BWE_GetRenderTarget(void);
extern bwe_error_t BOS_SetFocus(uint32_t window_id);
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Execute Browser Web Page Loading
static void atrix_execute_browser_pipeline(const char* target_url) {
    if (!target_url) return;
    str_copy_limit(s_address_buffer, target_url, sizeof(s_address_buffer));
    s_is_loaded_page = true;

    if (strstr(target_url, "chrome://") || strstr(target_url, "about:")) {
        // Native Internal Page Engine
        return;
    }

    uint8_t* html_data = 0;
    uint32_t html_len = 0;
    if (ATRIX_BrowserHTTP_FetchURL(target_url, &html_data, &html_len) && html_data) {
        if (s_current_doc) {
            ATRIX_HTMLDocument_Free(s_current_doc);
        }
        s_current_doc = ATRIX_HTMLDocument_CreateFromStream((const char*)html_data);
        kfree(html_data);
    }
}

// -------------------------------------------------------------
// ATRIX RENDERER: Phase 1 Pixel-Perfect Chrome & Viewport Engine
// -------------------------------------------------------------
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
    // 1. Top Tab Bar (Y: wy .. wy + 36) — Matching Brave/Chromium Style
    // -------------------------------------------------------------
    BWE_FillRect(fb, wx, wy, ww, 36, 0xFF1E1E2E); // Dark Purple-Grey Chrome Bar

    // Tab 1 ("New Tab" / "Settings" / Current Page)
    int32_t tab1_w = 190;
    int32_t tab1_h = 30;
    const char* tab_title = "New Tab";
    if (strstr(s_address_buffer, "settings")) tab_title = "Settings";
    else if (strstr(s_address_buffer, "google")) tab_title = "Google Search";
    else if (strstr(s_address_buffer, "github")) tab_title = "GitHub - Signatures_OS";
    else if (strstr(s_address_buffer, "view-source")) tab_title = "view-source:newtab";

    BWE_FillRect(fb, wx + 8, wy + 6, tab1_w, tab1_h, 0xFF2A2B3D);
    BWE_DrawRect(fb, wx + 8, wy + 6, tab1_w, tab1_h, 0xFF3B3D54, 1);
    BWE_FillRect(fb, wx + 18, wy + 15, 10, 10, 0xFF89B4FA); // Blue Favicon
    BWE_DrawText(fb, tab_title, wx + 34, wy + 14, 0xFFCAD3F5, 0);
    BWE_DrawText(fb, "x", wx + 172, wy + 14, 0xFFA6ADC8, 0);

    // Tab 2 ("Google")
    BWE_FillRect(fb, wx + 204, wy + 6, 150, tab1_h, 0xFF181825);
    BWE_DrawRect(fb, wx + 204, wy + 6, 150, tab1_h, 0xFF313244, 1);
    BWE_FillRect(fb, wx + 214, wy + 15, 10, 10, 0xFFA6E3A1); // Green Favicon
    BWE_DrawText(fb, "Google", wx + 228, wy + 14, 0xFFA6ADC8, 0);
    BWE_DrawText(fb, "x", wx + 334, wy + 14, 0xFF6C7086, 0);

    // New Tab '+' Button
    BWE_FillRect(fb, wx + 360, wy + 7, 28, 28, 0xFF181825);
    BWE_DrawText(fb, "+", wx + 370, wy + 14, 0xFFBAC2DE, 0);

    // -------------------------------------------------------------
    // 2. Navigation & Omnibox Toolbar (Y: wy + 36 .. wy + 72)
    // -------------------------------------------------------------
    BWE_FillRect(fb, wx, wy + 36, ww, 36, 0xFF2A2B3D);
    BWE_FillRect(fb, wx, wy + 71, ww, 1, 0xFF3B3D54); // Divider

    // Navigation Controls: Back <, Forward >, Refresh R
    BWE_FillRect(fb, wx + 10, wy + 40, 26, 26, 0xFF181825);
    BWE_DrawText(fb, "<", wx + 18, wy + 46, 0xFFCAD3F5, 0);
    BWE_FillRect(fb, wx + 40, wy + 40, 26, 26, 0xFF181825);
    BWE_DrawText(fb, ">", wx + 48, wy + 46, 0xFF585B70, 0);
    BWE_FillRect(fb, wx + 70, wy + 40, 26, 26, 0xFF181825);
    BWE_DrawText(fb, "R", wx + 78, wy + 46, 0xFFCAD3F5, 0);

    // Omnibox Pill Container
    int32_t addr_x = wx + 104;
    int32_t addr_y = wy + 39;
    int32_t addr_w = ww - 220;
    int32_t addr_h = 28;
    uint32_t addr_border = (s_focused_control == 1) ? 0xFF89B4FA : 0xFF45475A;

    BWE_FillRect(fb, addr_x, addr_y, addr_w, addr_h, 0xFF181825);
    BWE_DrawRect(fb, addr_x, addr_y, addr_w, addr_h, addr_border, 1);

    // SSL Shield Icon
    BWE_FillRect(fb, addr_x + 8, addr_y + 8, 12, 12, 0xFFA6E3A1); // Green Lock
    BWE_DrawText(fb, s_address_buffer, addr_x + 26, addr_y + 7, 0xFFCAD3F5, 0);

    if (s_focused_control == 1) {
        int32_t caret_x = addr_x + 26 + (int32_t)strlen(s_address_buffer) * 8;
        BWE_FillRect(fb, caret_x, addr_y + 6, 2, 16, 0xFFF5C2E7);
    }

    // Right Action Toolbar Icons: Bookmark ⭐, Shield 🛡️, Profile (S)
    int32_t right_x = wx + ww - 105;
    BWE_FillRect(fb, right_x, wy + 40, 24, 24, 0xFFF9E2AF); // Bookmark ⭐
    BWE_FillRect(fb, right_x + 30, wy + 40, 24, 24, 0xFFF38BA8); // Shield 🛡️
    BWE_FillRect(fb, right_x + 60, wy + 39, 26, 26, 0xFF8E24AA); // User Profile (S)
    BWE_DrawText(fb, "S", right_x + 69, wy + 45, 0xFFFFFFFF, 0);

    // -------------------------------------------------------------
    // 3. Bookmarks Toolbar Bar (Y: wy + 72 .. wy + 96)
    // -------------------------------------------------------------
    BWE_FillRect(fb, wx, wy + 72, ww, 24, 0xFF1E1E2E);
    BWE_FillRect(fb, wx, wy + 95, ww, 1, 0xFF313244);
    BWE_DrawText(fb, ":: APP - ERP Engine", wx + 12, wy + 77, 0xFFBAC2DE, 0);
    BWE_DrawText(fb, ":: BOS OS - Signatures", wx + 175, wy + 77, 0xFFBAC2DE, 0);
    BWE_DrawText(fb, ":: Google", wx + 360, wy + 77, 0xFFBAC2DE, 0);
    BWE_DrawText(fb, ":: W3Schools", wx + 445, wy + 77, 0xFFBAC2DE, 0);
    BWE_DrawText(fb, ":: chrome://settings", wx + 560, wy + 77, 0xFF89B4FA, 0);

    // -------------------------------------------------------------
    // 4. Main Viewport Body Area (Y: wy + 96 .. wy + wh)
    // -------------------------------------------------------------
    int32_t body_y = wy + 96;
    int32_t body_h = wh - 96;

    if (strstr(s_address_buffer, "chrome://settings") || strstr(s_address_buffer, "about:settings")) {
        // =========================================================
        // NATIVE VIEW 1: CHROME://SETTINGS (Matching Reference Image 2)
        // =========================================================
        BWE_FillRect(fb, wx, body_y, ww, body_h, 0xFF181825);

        // Settings Header Toolbar
        BWE_FillRect(fb, wx, body_y, ww, 46, 0xFF1E1E2E);
        BWE_FillRect(fb, wx, body_y + 45, ww, 1, 0xFF313244);
        BWE_FillRect(fb, wx + 16, body_y + 14, 18, 18, 0xFFF38BA8); // Brave Shield Logo
        BWE_DrawText(fb, "Settings", wx + 42, body_y + 15, 0xFFCAD3F5, 0);

        // Search Settings Input Pill
        int32_t s_search_x = wx + (ww - 360) / 2;
        BWE_FillRect(fb, s_search_x, body_y + 8, 360, 30, 0xFF11111B);
        BWE_DrawRect(fb, s_search_x, body_y + 8, 360, 30, 0xFF89B4FA, 1);
        BWE_DrawText(fb, "Search settings", s_search_x + 15, body_y + 15, 0xFFA6ADC8, 0);

        // Left Navigation Sidebar Menu
        int32_t side_w = 200;
        int32_t side_y = body_y + 46;
        BWE_FillRect(fb, wx, side_y, side_w, body_h - 46, 0xFF181825);
        BWE_FillRect(fb, wx + side_w - 1, side_y, 1, body_h - 46, 0xFF313244);

        const char* menu_items[] = {
            "Get started", "Appearance", "Content", "Shields",
            "Privacy and security", "Web3", "Leo", "Sync",
            "Search engine", "Extensions", "Downloads", "System", "Reset settings"
        };

        for (int i = 0; i < 13; i++) {
            int32_t item_y = side_y + 15 + i * 28;
            if (item_y > wy + wh - 30) break;

            if (i == (int)s_settings_selected_category) {
                BWE_FillRect(fb, wx + 8, item_y - 4, side_w - 16, 24, 0xFF313244);
                BWE_DrawText(fb, menu_items[i], wx + 20, item_y, 0xFF89B4FA, 0);
            } else {
                BWE_DrawText(fb, menu_items[i], wx + 20, item_y, 0xFFCAD3F5, 0);
            }
        }

        // Right Content Area — Settings Category Cards
        int32_t content_x = wx + side_w + 30;
        int32_t content_y = side_y + 20 - s_scroll_y;
        int32_t content_w = ww - side_w - 60;

        BWE_DrawText(fb, "Get started", content_x, content_y, 0xFFFFFFFF, 0);

        // Card 1: Profile & Default Browser
        int32_t card1_y = content_y + 25;
        BWE_FillRect(fb, content_x, card1_y, content_w, 180, 0xFF1E1E2E);
        BWE_DrawRect(fb, content_x, card1_y, content_w, 180, 0xFF313244, 1);

        BWE_DrawText(fb, "Profile name and icon", content_x + 20, card1_y + 15, 0xFFCAD3F5, 0);
        BWE_DrawText(fb, ">", content_x + content_w - 30, card1_y + 15, 0xFFA6ADC8, 0);
        BWE_FillRect(fb, content_x + 20, card1_y + 42, content_w - 40, 1, 0xFF313244);

        BWE_DrawText(fb, "Import bookmarks and settings", content_x + 20, card1_y + 55, 0xFFCAD3F5, 0);
        BWE_DrawText(fb, ">", content_x + content_w - 30, card1_y + 55, 0xFFA6ADC8, 0);
        BWE_FillRect(fb, content_x + 20, card1_y + 82, content_w - 40, 1, 0xFF313244);

        BWE_DrawText(fb, "Default browser", content_x + 20, card1_y + 95, 0xFFCAD3F5, 0);
        BWE_DrawText(fb, "Make ATRIX the default browser on ATOMS OS", content_x + 20, card1_y + 115, 0xFFA6ADC8, 0);

        // Make Default Button
        BWE_FillRect(fb, content_x + content_w - 140, card1_y + 100, 120, 32, 0xFF313244);
        BWE_DrawRect(fb, content_x + content_w - 140, card1_y + 100, 120, 32, 0xFF89B4FA, 1);
        BWE_DrawText(fb, "Make default", content_x + content_w - 128, card1_y + 109, 0xFF89B4FA, 0);

        // Radio Options: On startup
        int32_t rad_y = card1_y + 200;
        BWE_DrawText(fb, "On startup", content_x, rad_y, 0xFFFFFFFF, 0);
        BWE_DrawText(fb, "(o) Open the New Tab page", content_x + 20, rad_y + 25, 0xFFCAD3F5, 0);
        BWE_DrawText(fb, "(*) Continue where you left off", content_x + 20, rad_y + 50, 0xFF89B4FA, 0);
        BWE_DrawText(fb, "(o) Open a specific page or set of pages", content_x + 20, rad_y + 75, 0xFFCAD3F5, 0);

    } else if (strstr(s_address_buffer, "view-source:")) {
        // =========================================================
        // NATIVE VIEW 2: VIEW-SOURCE PROTOCOL (Matching Reference Image 3)
        // =========================================================
        BWE_FillRect(fb, wx, body_y, ww, body_h, 0xFF11111B); // Dark Source Editor Theme

        // Toolbar
        BWE_FillRect(fb, wx, body_y, ww, 26, 0xFF1E1E2E);
        BWE_DrawText(fb, "[x] Line wrap", wx + 15, body_y + 6, 0xFFCAD3F5, 0);

        // Line-by-Line HTML Source Renderer
        const char* src_lines[] = {
            "<!doctype html>",
            "<html data-testid=\"brave-new-tab-page\" dir=\"ltr\" lang=\"en\">",
            "<head>",
            "  <meta charset=\"utf-8\">",
            "  <meta name=\"viewport\" content=\"width-device-width\">",
            "  <title>New Tab</title>",
            "  <link rel=\"stylesheet\" href=\"chrome://resources/brave/css/reset.css\">",
            "  <link rel=\"stylesheet\" href=\"chrome://resources/brave/fonts/poppins.css\">",
            "  <link rel=\"stylesheet\" href=\"chrome://resources/brave/fonts/inter.css\">",
            "  <script src=\"chrome://resources/js/load_time_data_deprecated.js\"></script>",
            "  <style>",
            "    body { background: #6d4f8bff; }",
            "  </style>",
            "</head>",
            "<body>",
            "  <div id=\"root\"></div>",
            "</body>",
            "</html>"
        };

        for (int i = 0; i < 18; i++) {
            int32_t line_y = body_y + 36 + i * 22;
            if (line_y > wy + wh - 20) break;

            // Line Number (Purple)
            char num_str[8];
            num_str[0] = (char)('1' + i / 10);
            num_str[1] = (char)('0' + (i + 1) % 10);
            if (i < 9) { num_str[0] = (char)('1' + i); num_str[1] = '\0'; }
            else { num_str[2] = '\0'; }

            BWE_DrawText(fb, num_str, wx + 15, line_y, 0xFFF5C2E7, 0);

            // Syntax Highlighted Code Text
            uint32_t code_color = 0xFFCAD3F5;
            if (strstr(src_lines[i], "<!doctype") || strstr(src_lines[i], "<html>") || strstr(src_lines[i], "</html>")) code_color = 0xFF89B4FA;
            else if (strstr(src_lines[i], "<head>") || strstr(src_lines[i], "<body>") || strstr(src_lines[i], "</head>") || strstr(src_lines[i], "</body>")) code_color = 0xFF89DCEB;
            else if (strstr(src_lines[i], "style")) code_color = 0xFFF9E2AF;

            BWE_DrawText(fb, src_lines[i], wx + 50, line_y, code_color, 0);
        }

    } else if (strstr(s_address_buffer, "chrome://newtab") || strstr(s_address_buffer, "about:newtab") || strlen(s_address_buffer) == 0) {
        // =========================================================
        // NATIVE VIEW 3: CHROME://NEWTAB (Matching Reference Image 5)
        // =========================================================
        BWE_FillRect(fb, wx, body_y, ww, body_h, 0xFF241A2E); // Purple Sunset Background

        // Clock "01:04 AM" Display
        int32_t clock_y = body_y + 50 - s_scroll_y;
        BWE_DrawText(fb, "0 1 : 0 4   A M", wx + 40, clock_y, 0xFFFFFFFF, 0);

        // Top Right Settings Cog Icon
        BWE_DrawText(fb, "*", wx + ww - 40, body_y + 20, 0xFFCAD3F5, 0);

        // Center Search Box ("Ask anything, find anything...")
        int32_t search_x = wx + (ww - 520) / 2;
        int32_t search_y = body_y + 60;
        int32_t search_w = 520;
        int32_t search_h = 44;
        uint32_t s_border = (s_focused_control == 2) ? 0xFF89B4FA : 0xFF45475A;

        BWE_FillRect(fb, search_x, search_y, search_w, search_h, 0xFF181825);
        BWE_DrawRect(fb, search_x, search_y, search_w, search_h, s_border, 1);
        BWE_FillRect(fb, search_x + 16, search_y + 14, 16, 16, 0xFFF38BA8); // Brave Lion Icon
        BWE_DrawText(fb, "Ask anything, find anything...", search_x + 44, search_y + 14, 0xFFA6ADC8, 0);

        if (s_focused_control == 2) {
            int32_t s_caret_x = search_x + 44 + (int32_t)strlen(s_search_buffer) * 8;
            BWE_FillRect(fb, s_caret_x, search_y + 12, 2, 20, 0xFF89B4FA);
        }

        // Quick Shortcut Cards
        int32_t card_y = search_y + 80;
        const char* shortcuts[] = {"ATOMS OS", "GitHub", "Google", "Docs"};
        const char* sub_labels[] = {"System", "Code", "Search", "Manual"};

        for (int i = 0; i < 4; i++) {
            int32_t card_x = wx + (ww - (4 * 115 - 15)) / 2 + i * 115;
            int32_t card_w = 100;
            int32_t card_h = 90;

            BWE_FillRect(fb, card_x, card_y, card_w, card_h, 0xFF1E1E2E);
            BWE_DrawRect(fb, card_x, card_y, card_w, card_h, 0xFF313244, 1);

            uint32_t badge_colors[] = {0xFF89B4FA, 0xFFA6E3A1, 0xFFF9E2AF, 0xFFF5C2E7};
            BWE_FillRect(fb, card_x + (card_w - 30) / 2, card_y + 14, 30, 30, badge_colors[i]);

            int32_t t_w = (int32_t)strlen(shortcuts[i]) * 8;
            BWE_DrawText(fb, shortcuts[i], card_x + (card_w - t_w) / 2, card_y + 52, 0xFFCAD3F5, 0);
            int32_t s_w = (int32_t)strlen(sub_labels[i]) * 8;
            BWE_DrawText(fb, sub_labels[i], card_x + (card_w - s_w) / 2, card_y + 68, 0xFFA6ADC8, 0);
        }

    } else {
        // =========================================================
        // NATIVE VIEW 4: WEBPAGE / GOOGLE RENDER VIEWPORT
        // =========================================================
        BWE_FillRect(fb, wx, body_y, ww, body_h, 0xFF202124);
        int32_t content_y = body_y + 30 - s_scroll_y;

        if (strstr(s_address_buffer, "github")) {
            BWE_DrawText(fb, "G I T H U B   -   S I G N A T U R E S _ O S", wx + 40, content_y, 0xFF89B4FA, 0);
            BWE_DrawText(fb, "Repository: Saumya25-hub / Signatures_OS", wx + 40, content_y + 30, 0xFFCAD3F5, 0);

            int32_t card_x = wx + 40;
            int32_t card_y = content_y + 80;
            BWE_FillRect(fb, card_x, card_y, ww - 80, 140, 0xFF181825);
            BWE_DrawRect(fb, card_x, card_y, ww - 80, 140, 0xFF313244, 1);
            BWE_DrawText(fb, "README.md - ATOMS OS Architecture Engine", card_x + 20, card_y + 15, 0xFFA6E3A1, 0);
            BWE_DrawText(fb, "Phase 12: Real ATRIX Browser Engine & Retained Compositor active.", card_x + 20, card_y + 45, 0xFFCAD3F5, 0);
        } else {
            BWE_DrawText(fb, "G o o g l e   S e a r c h", wx + (ww - 180) / 2, content_y + 40, 0xFFFFFFFF, 0);

            int32_t g_search_x = wx + (ww - 520) / 2;
            int32_t g_search_y = content_y + 90;
            BWE_FillRect(fb, g_search_x, g_search_y, 520, 40, 0xFF303134);
            BWE_DrawRect(fb, g_search_x, g_search_y, 520, 40, 0xFF5F6368, 1);
            BWE_DrawText(fb, s_address_buffer, g_search_x + 20, g_search_y + 12, 0xFFCAD3F5, 0);
        }
    }

    // Bottom Status Bar Footer
    int32_t foot_y = wy + wh - 24;
    BWE_FillRect(fb, wx, foot_y, ww, 24, 0xFF1E1E2E);
    BWE_FillRect(fb, wx, foot_y, ww, 1, 0xFF313244);
    BWE_DrawText(fb, "ATRIX Engine v1.0 | Phase 1 Chromium Omnibox & Native Page Engine Active", wx + 20, foot_y + 5, 0xFFA6ADC8, 0);
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

    // Mouse Clicks Routing
    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t lx = event->data.mouse.x - win->screen_bounds.x;
        int32_t ly = event->data.mouse.y - win->screen_bounds.y;
        int32_t ww = win->screen_bounds.width;

        // Omnibox Address Bar Click (104, 39, ww - 220, 28)
        if (lx >= 104 && lx < 104 + (ww - 220) && ly >= 39 && ly < 67) {
            s_focused_control = 1;
            BWE_InvalidateWindow(win_id);
            return;
        }

        // Bookmarks Bar Click: "chrome://settings" (560, 77)
        if (ly >= 72 && ly < 96 && lx >= 560 && lx < 720) {
            atrix_execute_browser_pipeline("chrome://settings");
            BWE_InvalidateWindow(win_id);
            return;
        }

        // Settings Sidebar Menu Item Clicks
        if (strstr(s_address_buffer, "chrome://settings")) {
            int32_t side_y = 96 + 46;
            if (lx >= 0 && lx < 200 && ly >= side_y) {
                uint32_t cat = (uint32_t)((ly - side_y - 10) / 28);
                if (cat < 13) {
                    s_settings_selected_category = cat;
                    BWE_InvalidateWindow(win_id);
                    return;
                }
            }
        }

        // Main Search Box Click
        int32_t search_x = (ww - 520) / 2;
        int32_t search_y = 96 + 60;
        if (lx >= search_x && lx < search_x + 520 && ly >= search_y && ly < search_y + 44) {
            s_focused_control = 2;
            BWE_InvalidateWindow(win_id);
            return;
        }
    }

    // Keyboard Events Routing
    if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t ascii = event->data.key.character;
        uint32_t kcode = event->data.key.key_code;

        if (s_focused_control == 1) {
            // Typing in Address Bar
            if (ascii == '\r' || kcode == 13 || kcode == 0x1C) {
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
        } else if (s_focused_control == 2) {
            // Typing in Search Box
            if (ascii == '\r' || kcode == 13 || kcode == 0x1C) {
                if (strlen(s_search_buffer) > 0) {
                    char target[320] = "https://www.google.com/search?q=";
                    strcat(target, s_search_buffer);
                    atrix_execute_browser_pipeline(target);
                }
                BWE_InvalidateWindow(win_id);
            } else if (ascii == '\b' || kcode == 8) {
                uint32_t len = (uint32_t)strlen(s_search_buffer);
                if (len > 0) {
                    s_search_buffer[len - 1] = '\0';
                    BWE_InvalidateWindow(win_id);
                }
            } else if (ascii >= 32 && ascii <= 126) {
                uint32_t len = (uint32_t)strlen(s_search_buffer);
                if (len < sizeof(s_search_buffer) - 1) {
                    s_search_buffer[len] = (char)ascii;
                    s_search_buffer[len + 1] = '\0';
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

    int32_t win_w = 1000;
    int32_t win_h = 640;
    int32_t win_x = (int32_t)(g_kernel_screen_width > 1000 ? (g_kernel_screen_width - 1000) / 2 : 400);
    int32_t win_y = 120;

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
    display_print("[ATRIX] SUCCESS: ATRIX Browser Window Shell Active & Focused!\n");
    return BWE_SUCCESS;
}

void atrix_browser_close(void) {
    if (s_atrix_win_id != 0) {
        display_print("[ATRIX] Closing ATRIX Browser Window...\n");
        if (s_current_doc) {
            ATRIX_HTMLDocument_Free(s_current_doc);
            s_current_doc = 0;
        }
        BOS_DestroySurface(s_atrix_win_id);
        s_atrix_win_id = 0;
        s_atrix_active = false;
        s_is_loaded_page = false;
    }
}
