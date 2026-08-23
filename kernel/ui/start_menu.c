#include "start_menu.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/ui/bofont/bofont.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/core/lib/include/string.h"

uint32_t g_start_menu_win_id = 0;
bool g_start_menu_open = false;

static char s_search_query[64] = "";
static bool s_power_flyout_open = false;
static int32_t s_hover_index = -1; // -1 = none, 0..N = App cards, 100 = Search field, 101 = Power btn, 200..202 = Flyout items

// Cached App Entry Structure
typedef struct {
    uint32_t app_id;
    const char* display_name;
    uint32_t asset_id;
} StartMenuCachedApp;

static StartMenuCachedApp s_app_cache[24];
static uint32_t s_cached_app_count = 0;

static uint32_t get_asset_for_app_id(uint32_t app_id) {
    switch (app_id) {
        case APP_ID_EXPLORER:    return ICON_EXPLORER;
        case APP_ID_NOTES:       return ICON_FILE;
        case APP_ID_CALCULATOR:  return ICON_CALCULATOR;
        case APP_ID_TERMINAL:    return ICON_TERMINAL;
        case APP_ID_SETTINGS:    return ICON_SETTINGS;
        case APP_ID_MUSIC:       return ICON_MUSIC;
        case APP_ID_ATRIX:       return ICON_ATRIX;
        case APP_ID_TMH:         return ICON_TMH;
        case APP_ID_CONTROLPANEL:return ICON_SETTINGS;
        case APP_ID_DOOM:        return ICON_DOOM;
        case APP_ID_GRAPH_3D:    return ICON_GRAPH_3D;
        case APP_ID_STRESS_TEST: return ICON_STRESS_TEST;
        case APP_ID_INPUT_LAB:   return ICON_INPUT_LAB;
        case APP_ID_IMAGE_VIEWER:return ICON_FILE;
        case APP_ID_SANDBOX:     return ICON_FILE;
        default:                 return ICON_FILE;
    }
}

// Refresh Static App Cache from Horse Engine
void StartMenu_RefreshCache(void) {
    uint32_t reg_count = 0;
    HorseAppEntry* reg_apps = horse_get_running(&reg_count);
    if (!reg_apps || reg_count == 0) return;

    s_cached_app_count = 0;
    for (uint32_t i = 0; i < reg_count && s_cached_app_count < 24; i++) {
        uint32_t id = reg_apps[i].app_id;
        // Enumerate primary ATOMS desktop applications
        if (id == APP_ID_EXPLORER || id == APP_ID_NOTES || id == APP_ID_CALCULATOR ||
            id == APP_ID_TERMINAL || id == APP_ID_SETTINGS || id == APP_ID_MUSIC ||
            id == APP_ID_ATRIX || id == APP_ID_TMH || id == APP_ID_CONTROLPANEL ||
            id == APP_ID_DOOM || id == APP_ID_GRAPH_3D) {
            s_app_cache[s_cached_app_count].app_id = id;
            s_app_cache[s_cached_app_count].display_name = reg_apps[i].display_name;
            s_app_cache[s_cached_app_count].asset_id = get_asset_for_app_id(id);
            s_cached_app_count++;
        }
    }
}

// Substring matching without heap allocation
static bool contains_str_nocase(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    if (needle[0] == '\0') return true;

    int hlen = strlen(haystack);
    int nlen = strlen(needle);
    if (nlen > hlen) return false;

    for (int i = 0; i <= hlen - nlen; i++) {
        bool match = true;
        for (int j = 0; j < nlen; j++) {
            char c1 = haystack[i + j];
            char c2 = needle[j];
            if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
            if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
            if (c1 != c2) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

// Clipped Pixel Plotting Helper
static inline void plot_pixel(const BVFramebuffer* fb, int32_t x, int32_t y, uint32_t color, const BWE_Rect* clip) {
    if (x >= clip->x && x < clip->x + clip->width && y >= clip->y && y < clip->y + clip->height) {
        if (x >= 0 && x < (int32_t)fb->width && y >= 0 && y < (int32_t)fb->height) {
            uint32_t alpha = (color >> 24) & 0xFF;
            if (alpha == 0xFF) {
                fb->buffer[y * (fb->pitch / 4) + x] = color;
            } else if (alpha > 0) {
                uint32_t dst = fb->buffer[y * (fb->pitch / 4) + x];
                uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
                uint32_t sr = (color >> 16) & 0xFF, sg = (color >> 8) & 0xFF, sb = color & 0xFF;
                uint32_t r = (sr * alpha + dr * (255 - alpha)) / 255;
                uint32_t g = (sg * alpha + dg * (255 - alpha)) / 255;
                uint32_t b = (sb * alpha + db * (255 - alpha)) / 255;
                fb->buffer[y * (fb->pitch / 4) + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
}

// Rounded rectangle boundary checker
static inline bool is_outside_rounded_rect(int32_t x, int32_t y, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r) {
    int32_t left_cx = rx + r;
    int32_t right_cx = rx + rw - r - 1;
    int32_t top_cy = ry + r;
    int32_t bottom_cy = ry + rh - r - 1;

    if (x < left_cx && y < top_cy) {
        int32_t dx = x - left_cx;
        int32_t dy = y - top_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    if (x > right_cx && y < top_cy) {
        int32_t dx = x - right_cx;
        int32_t dy = y - top_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    if (x < left_cx && y > bottom_cy) {
        int32_t dx = x - left_cx;
        int32_t dy = y - bottom_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    if (x > right_cx && y > bottom_cy) {
        int32_t dx = x - right_cx;
        int32_t dy = y - bottom_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    return false;
}

// UI Primitives
static void draw_rounded_panel(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, uint32_t bg_color, uint32_t border_color, const BWE_Rect* clip) {
    for (int32_t y = ry - 2; y < ry + rh + 4; y++) {
        for (int32_t x = rx - 2; x < rx + rw + 4; x++) {
            if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) {
                if (!is_outside_rounded_rect(x - 2, y - 3, rx, ry, rw, rh, r)) {
                    plot_pixel(fb, x, y, 0x1A000000, clip);
                } else if (!is_outside_rounded_rect(x - 4, y - 5, rx, ry, rw, rh, r)) {
                    plot_pixel(fb, x, y, 0x0D000000, clip);
                }
                continue;
            }

            bool is_edge = (x == rx || x == rx + rw - 1 || y == ry || y == ry + rh - 1 ||
                            is_outside_rounded_rect(x - 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x + 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y - 1, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y + 1, rx, ry, rw, rh, r));

            plot_pixel(fb, x, y, is_edge ? border_color : bg_color, clip);
        }
    }
}

static void draw_rounded_box(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, uint32_t color, const BWE_Rect* clip) {
    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            if (!is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) {
                plot_pixel(fb, x, y, color, clip);
            }
        }
    }
}

static void draw_separator_line(const BVFramebuffer* fb, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color, const BWE_Rect* clip) {
    if (y1 == y2) {
        for (int32_t x = x1; x <= x2; x++) plot_pixel(fb, x, y1, color, clip);
    } else if (x1 == x2) {
        for (int32_t y = y1; y <= y2; y++) plot_pixel(fb, x1, y, color, clip);
    }
}

static void draw_search_field(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, const char* text, bool is_focused, const BWE_Rect* clip) {
    uint32_t bg_col = is_focused ? 0xFF1E293B : 0xFF182234;
    uint32_t border_col = is_focused ? 0xFF3B82F6 : 0xFF334155;

    draw_rounded_box(fb, rx, ry, rw, rh, r, bg_col, clip);

    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) continue;
            bool is_edge = (x == rx || x == rx + rw - 1 || y == ry || y == ry + rh - 1 ||
                            is_outside_rounded_rect(x - 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x + 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y - 1, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y + 1, rx, ry, rw, rh, r));
            if (is_edge) plot_pixel(fb, x, y, border_col, clip);
        }
    }

    BWE_DrawText(fb, "Q", rx + 14, ry + 12, 0xFF94A3B8, 0);

    if (text[0] != '\0') {
        char disp_text[60];
        strncpy(disp_text, text, 55);
        disp_text[55] = '\0';
        int len = strlen(disp_text);
        if (is_focused) {
            disp_text[len] = '|';
            disp_text[len + 1] = '\0';
        }
        BWE_DrawText(fb, disp_text, rx + 38, ry + 12, 0xFFF1F5F9, 0);
    } else {
        const char* ph = is_focused ? "|" : "Search apps, settings, and files...";
        BWE_DrawText(fb, ph, rx + 38, ry + 12, 0xFF64748B, 0);
    }
}

// Procedural Power Symbol Icon (20x20)
static void draw_power_icon(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    for (int32_t y = oy + 2; y <= oy + 9; y++) {
        plot_pixel(fb, ox + 9, y, color, clip);
        plot_pixel(fb, ox + 10, y, color, clip);
    }
    float cx = (float)ox + 9.5f;
    float cy = (float)oy + 10.5f;
    for (int32_t y = oy + 3; y <= oy + 17; y++) {
        for (int32_t x = ox + 3; x <= ox + 16; x++) {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float r_sq = dx*dx + dy*dy;
            if (r_sq >= 4.5f * 4.5f && r_sq <= 7.2f * 7.2f) {
                if (!(dy < 0 && dx >= -3.5f && dx <= 3.5f)) {
                    plot_pixel(fb, x, y, color, clip);
                }
            }
        }
    }
}

// Procedural Restart Circular Arrow Icon (20x20)
static void draw_restart_icon(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    float cx = (float)ox + 9.5f;
    float cy = (float)oy + 9.5f;
    for (int32_t y = oy + 2; y <= oy + 17; y++) {
        for (int32_t x = ox + 2; x <= ox + 17; x++) {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float r_sq = dx*dx + dy*dy;
            if (r_sq >= 4.5f * 4.5f && r_sq <= 7.2f * 7.2f) {
                if (!(dy < -2.0f && dx > 1.0f)) {
                    plot_pixel(fb, x, y, color, clip);
                }
            }
        }
    }
    plot_pixel(fb, ox + 12, oy + 2, color, clip);
    plot_pixel(fb, ox + 13, oy + 3, color, clip);
    plot_pixel(fb, ox + 14, oy + 4, color, clip);
    plot_pixel(fb, ox + 13, oy + 5, color, clip);
    plot_pixel(fb, ox + 14, oy + 2, color, clip);
    plot_pixel(fb, ox + 15, oy + 3, color, clip);
}

// Procedural Crescent Moon Sleep Icon (20x20)
static void draw_sleep_icon(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    float cx1 = (float)ox + 8.5f;
    float cy1 = (float)oy + 9.5f;
    float cx2 = (float)ox + 12.0f;
    float cy2 = (float)oy + 7.5f;

    for (int32_t y = oy + 2; y <= oy + 17; y++) {
        for (int32_t x = ox + 2; x <= ox + 17; x++) {
            float dx1 = (float)x - cx1;
            float dy1 = (float)y - cy1;
            float r1_sq = dx1*dx1 + dy1*dy1;

            float dx2 = (float)x - cx2;
            float dy2 = (float)y - cy2;
            float r2_sq = dx2*dx2 + dy2*dy2;

            if (r1_sq <= 7.5f * 7.5f && r2_sq >= 5.5f * 5.5f) {
                plot_pixel(fb, x, y, color, clip);
            }
        }
    }
}

static void draw_power_flyout(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t hover_item, const BWE_Rect* clip) {
    uint32_t bg_col = 0xFA0F172A;
    uint32_t border_col = 0xFF334155;

    draw_rounded_panel(fb, rx, ry, rw, rh, 10, bg_col, border_col, clip);

    const char* items[] = { "Sleep", "Restart", "Power Off" };
    for (int i = 0; i < 3; i++) {
        int32_t item_y = ry + 8 + i * 36;
        int32_t item_w = rw - 16;
        int32_t item_h = 32;

        if (hover_item == i) {
            draw_rounded_box(fb, rx + 8, item_y, item_w, item_h, 6, 0x383B82F6, clip);
        }

        if (i == 0) {
            draw_sleep_icon(fb, rx + 14, item_y + 6, 0xFF38BDF8, clip);
        } else if (i == 1) {
            draw_restart_icon(fb, rx + 14, item_y + 6, 0xFFFACC15, clip);
        } else {
            draw_power_icon(fb, rx + 14, item_y + 6, 0xFFEF4444, clip);
        }

        BWE_DrawText(fb, items[i], rx + 40, item_y + 8, 0xFFF1F5F9, 0);
    }
}

// Render Callback
static void start_menu_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    int32_t abs_px = self->screen_bounds.x;
    int32_t abs_py = self->screen_bounds.y;
    int32_t panel_w = self->screen_bounds.width;
    int32_t panel_h = self->screen_bounds.height;

    uint32_t bg_col     = BOTHEME_GetColor(BOTHEME_TASKBAR_BG);
    uint32_t border_col = BOTHEME_GetColor(BOTHEME_TASKBAR_BORDER);

    if ((bg_col & 0x00FFFFFF) != 0) {
        bg_col = 0xFA000000 | (bg_col & 0x00FFFFFF);
    } else {
        bg_col = 0xFA0F172A;
    }
    if ((border_col & 0x00FFFFFF) == 0) {
        border_col = 0xFF334155;
    }

    // 1. Outer Panel
    draw_rounded_panel(fb, abs_px, abs_py, panel_w, panel_h, 16, bg_col, border_col, &clip);

    // 2. Search Field
    int32_t search_x = abs_px + 24;
    int32_t search_y = abs_py + 18;
    int32_t search_w = panel_w - 48;
    int32_t search_h = 38;
    bool search_focused = (s_hover_index == 100);

    draw_search_field(fb, search_x, search_y, search_w, search_h, 10, s_search_query, search_focused, &clip);

    // 3. Section Header
    int32_t content_y = search_y + search_h + 14;
    if (s_search_query[0] == '\0') {
        BWE_DrawTextRole(fb, "Pinned Applications", abs_px + 24, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
    } else {
        BWE_DrawTextRole(fb, "Search Results", abs_px + 24, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
    }

    // 4. Render App Grid from Cached Table
    if (s_cached_app_count == 0) {
        StartMenu_RefreshCache();
    }

    int32_t grid_start_x = abs_px + 24;
    int32_t grid_start_y = content_y + 22;
    int32_t card_w = 126;
    int32_t card_h = 72;
    int32_t cols = 4;
    int32_t spacing_x = 12;
    int32_t spacing_y = 10;

    uint32_t visible_count = 0;
    for (uint32_t i = 0; i < s_cached_app_count && visible_count < 12; i++) {
        if (s_search_query[0] != '\0' && !contains_str_nocase(s_app_cache[i].display_name, s_search_query)) {
            continue;
        }

        int32_t col = visible_count % cols;
        int32_t row = visible_count / cols;
        int32_t cx = grid_start_x + col * (card_w + spacing_x);
        int32_t cy = grid_start_y + row * (card_h + spacing_y);

        bool is_card_hovered = (s_hover_index == (int32_t)visible_count);

        if (is_card_hovered) {
            draw_rounded_box(fb, cx, cy, card_w, card_h, 10, 0x33FFFFFF, &clip);
        } else {
            draw_rounded_box(fb, cx, cy, card_w, card_h, 10, 0x1A1E293B, &clip);
        }

        int32_t ix = cx + (card_w - 32) / 2;
        int32_t iy = cy + 8;
        if (!BOAsset_DrawAsset(s_app_cache[i].asset_id, ix, iy, 32, 32)) {
            BWE_FillRect(fb, ix, iy, 32, 32, 0xFF3B82F6);
        }

        BOTextMetrics tm = BOFont_MeasureTextRole(BOFONT_ROLE_UI_MEDIUM, s_app_cache[i].display_name);
        int32_t text_x = cx + (card_w - tm.width) / 2;
        if (text_x < cx + 4) text_x = cx + 4;
        BWE_DrawTextRole(fb, s_app_cache[i].display_name, text_x, cy + 46, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);

        visible_count++;
    }

    if (visible_count == 0 && s_search_query[0] != '\0') {
        BWE_DrawTextRole(fb, "No matching applications found.", abs_px + 24, grid_start_y + 20, 0xFF64748B, BOFONT_ROLE_UI_REGULAR);
    }

    // 5. Footer
    int32_t footer_y = abs_py + panel_h - 52;
    draw_separator_line(fb, abs_px + 16, footer_y, abs_px + panel_w - 16, footer_y, 0x33FFFFFF, &clip);

    int32_t av_x = abs_px + 24;
    int32_t av_y = footer_y + 8;
    draw_rounded_box(fb, av_x, av_y, 36, 36, 18, 0xFF2563EB, &clip);
    BWE_DrawTextRole(fb, "S", av_x + 14, av_y + 10, 0xFFFFFFFF, BOFONT_ROLE_UI_BOLD);
    BWE_DrawTextRole(fb, "Saumya", av_x + 48, av_y + 10, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);

    int32_t pwr_x = abs_px + panel_w - 60;
    int32_t pwr_y = footer_y + 8;
    bool pwr_hovered = (s_hover_index == 101);

    if (pwr_hovered || s_power_flyout_open) {
        draw_rounded_box(fb, pwr_x, pwr_y, 36, 36, 10, s_power_flyout_open ? 0x66EF4444 : 0x33FFFFFF, &clip);
    }
    draw_power_icon(fb, pwr_x + 8, pwr_y + 8, 0xFFEF4444, &clip);

    // 6. Power Flyout
    if (s_power_flyout_open) {
        int32_t flyout_w = 160;
        int32_t flyout_h = 120;
        int32_t flyout_x = abs_px + panel_w - 180;
        int32_t flyout_y = footer_y - flyout_h - 8;

        int32_t flyout_hover = (s_hover_index >= 200 && s_hover_index <= 202) ? (s_hover_index - 200) : -1;
        draw_power_flyout(fb, flyout_x, flyout_y, flyout_w, flyout_h, flyout_hover, &clip);
    }
}

// Event Callback
static void start_menu_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    int32_t abs_px = self->screen_bounds.x;
    int32_t abs_py = self->screen_bounds.y;
    int32_t panel_w = self->screen_bounds.width;
    int32_t panel_h = self->screen_bounds.height;
    int32_t footer_y = abs_py + panel_h - 52;

    if (event->type == BWE_EVENT_MOUSE_MOVE) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        int32_t new_hover = -1;

        if (s_power_flyout_open) {
            int32_t flyout_w = 160;
            int32_t flyout_h = 120;
            int32_t flyout_x = abs_px + panel_w - 180;
            int32_t flyout_y = footer_y - flyout_h - 8;

            if (mx >= flyout_x && mx < flyout_x + flyout_w && my >= flyout_y && my < flyout_y + flyout_h) {
                int item = (my - (flyout_y + 8)) / 36;
                if (item >= 0 && item <= 2) new_hover = 200 + item;
            }
        }

        if (new_hover == -1) {
            if (mx >= abs_px + 24 && mx < abs_px + panel_w - 24 && my >= abs_py + 18 && my < abs_py + 56) {
                new_hover = 100;
            } else if (mx >= abs_px + panel_w - 60 && mx < abs_px + panel_w - 24 && my >= footer_y + 8 && my < footer_y + 44) {
                new_hover = 101;
            } else {
                int32_t grid_start_x = abs_px + 24;
                int32_t grid_start_y = abs_py + 94;
                int32_t card_w = 126;
                int32_t card_h = 72;
                int32_t cols = 4;
                int32_t spacing_x = 12;
                int32_t spacing_y = 10;

                uint32_t visible_count = 0;
                for (uint32_t i = 0; i < s_cached_app_count && visible_count < 12; i++) {
                    if (s_search_query[0] != '\0' && !contains_str_nocase(s_app_cache[i].display_name, s_search_query)) {
                        continue;
                    }
                    int32_t col = visible_count % cols;
                    int32_t row = visible_count / cols;
                    int32_t cx = grid_start_x + col * (card_w + spacing_x);
                    int32_t cy = grid_start_y + row * (card_h + spacing_y);

                    if (mx >= cx && mx < cx + card_w && my >= cy && my < cy + card_h) {
                        new_hover = (int32_t)visible_count;
                        break;
                    }
                    visible_count++;
                }
            }
        }

        if (new_hover != s_hover_index) {
            s_hover_index = new_hover;
            BWE_InvalidateWindow(window_id);
        }
        return;
    }

    if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t key = event->data.key.key_code;
        uint32_t ch  = event->data.key.character;

        if (key == 27) { // Escape key
            s_power_flyout_open = false;
            s_search_query[0] = '\0';
            g_start_menu_open = false;
            BOS_Hide(window_id);
            extern uint32_t g_task_panel_win_id;
            if (g_task_panel_win_id) BWE_InvalidateWindow(g_task_panel_win_id);
            return;
        }

        if (key == 8 || key == 127) { // Backspace
            int len = strlen(s_search_query);
            if (len > 0) {
                s_search_query[len - 1] = '\0';
                BWE_InvalidateWindow(window_id);
            }
            return;
        }

        if (ch >= 32 && ch <= 126) { // Printable ASCII
            int len = strlen(s_search_query);
            if (len < 50) {
                s_search_query[len] = (char)ch;
                s_search_query[len + 1] = '\0';
                BWE_InvalidateWindow(window_id);
            }
            return;
        }
    }

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        if (mx < abs_px || mx >= abs_px + panel_w || my < abs_py || my >= abs_py + panel_h) {
            s_power_flyout_open = false;
            g_start_menu_open = false;
            BOS_Hide(window_id);
            extern uint32_t g_task_panel_win_id;
            if (g_task_panel_win_id) BWE_InvalidateWindow(g_task_panel_win_id);
            return;
        }

        if (s_power_flyout_open) {
            int32_t flyout_w = 160;
            int32_t flyout_h = 120;
            int32_t flyout_x = abs_px + panel_w - 180;
            int32_t flyout_y = footer_y - flyout_h - 8;

            if (mx >= flyout_x && mx < flyout_x + flyout_w && my >= flyout_y && my < flyout_y + flyout_h) {
                int item = (my - (flyout_y + 8)) / 36;
                s_power_flyout_open = false;
                g_start_menu_open = false;
                BOS_Hide(window_id);

                if (item == 1) {
                    horse_restart();
                } else if (item == 2) {
                    horse_shutdown();
                }
                return;
            } else {
                s_power_flyout_open = false;
            }
        }

        if (mx >= abs_px + panel_w - 60 && mx < abs_px + panel_w - 24 && my >= footer_y + 8 && my < footer_y + 44) {
            s_power_flyout_open = !s_power_flyout_open;
            BWE_InvalidateWindow(window_id);
            return;
        }

        int32_t grid_start_x = abs_px + 24;
        int32_t grid_start_y = abs_py + 94;
        int32_t card_w = 126;
        int32_t card_h = 72;
        int32_t cols = 4;
        int32_t spacing_x = 12;
        int32_t spacing_y = 10;

        uint32_t visible_count = 0;
        for (uint32_t i = 0; i < s_cached_app_count && visible_count < 12; i++) {
            if (s_search_query[0] != '\0' && !contains_str_nocase(s_app_cache[i].display_name, s_search_query)) {
                continue;
            }
            int32_t col = visible_count % cols;
            int32_t row = visible_count / cols;
            int32_t cx = grid_start_x + col * (card_w + spacing_x);
            int32_t cy = grid_start_y + row * (card_h + spacing_y);

            if (mx >= cx && mx < cx + card_w && my >= cy && my < cy + card_h) {
                uint32_t target_app_id = s_app_cache[i].app_id;

                s_power_flyout_open = false;
                s_search_query[0] = '\0';
                g_start_menu_open = false;
                BOS_Hide(window_id);

                horse_launch(target_app_id);

                extern uint32_t g_task_panel_win_id;
                if (g_task_panel_win_id) BWE_InvalidateWindow(g_task_panel_win_id);
                return;
            }
            visible_count++;
        }
    }
}

void StartMenu_Initialize(void) {
    const AGDAE_Metrics* metrics = AGDAE_GetMetrics();
    int32_t sw = metrics->desktop_rect.width;
    int32_t sh = metrics->desktop_rect.height;
    
    int32_t panel_w = 580;
    int32_t panel_h = 420;
    int32_t start_x = (sw - panel_w) / 2;
    int32_t start_y = sh - 66 - panel_h - 10;

    StartMenu_RefreshCache();

    BOS_CreatePanel(BWE_DESKTOP_ID, start_x, start_y, panel_w, panel_h, 0x00000000, &g_start_menu_win_id);
    BWE_Window* sm = BWE_GetWindow(g_start_menu_win_id);
    if (sm) {
        sm->type = BWE_TYPE_PANEL;
        sm->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST | BWE_WINDOW_TRANSPARENT;
        sm->on_render = start_menu_render_callback;
        sm->on_event = start_menu_event_callback;
        BOS_Hide(g_start_menu_win_id);
        g_start_menu_open = false;
    }
}
