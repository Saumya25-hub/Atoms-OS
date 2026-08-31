#include "start_menu.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/ui/bofont/bofont.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/gui/surface/surface.h"

uint32_t g_start_menu_win_id = 0;
bool g_start_menu_open = false;

/* ========================================================================= */
/* Start Menu Categories & Static Application Registry                       */
/* ========================================================================= */

typedef enum {
    START_CAT_ALL = 0,
    START_CAT_FILES,
    START_CAT_DEVELOPMENT,
    START_CAT_INTERNET,
    START_CAT_SYSTEM,
    START_CAT_MEDIA,
    START_CAT_COUNT
} StartMenuCategory;

typedef struct {
    StartMenuCategory category;
    const char* name;
    const char* glyph;
} StartMenuCategoryInfo;

static const StartMenuCategoryInfo s_categories[START_CAT_COUNT] = {
    { START_CAT_ALL,         "All Apps",         "*" },
    { START_CAT_FILES,       "Files & Docs",     "F" },
    { START_CAT_DEVELOPMENT, "Development",      ">" },
    { START_CAT_INTERNET,    "Internet & Web",   "W" },
    { START_CAT_SYSTEM,      "System & Config",  "S" },
    { START_CAT_MEDIA,       "Entertainment",    "M" },
};

typedef struct {
    uint32_t app_id;
    const char* display_name;
    const char* category_label;
    uint32_t asset_id;
    StartMenuCategory category;
    bool is_pinned;
} StartMenuAppEntry;

static StartMenuAppEntry s_app_cache[24];
static uint32_t s_cached_app_count = 0;

static StartMenuCategory s_active_category = START_CAT_ALL;
static char s_search_query[64] = "";
static bool s_power_flyout_open = false;
static int32_t s_hover_index = -1; 
/*
 * s_hover_index encoding:
 * -1        = None
 * 0..23     = Filtered app card index
 * 100       = Search field
 * 110..115  = Category navigation items (110 + category_id)
 * 150       = Settings shortcut button
 * 151       = Power action button
 * 160       = User profile pill
 * 200..202  = Power flyout items (0=Sleep, 1=Restart, 2=Power Off)
 */

static uint32_t get_asset_for_app_id(uint32_t app_id) {
    switch (app_id) {
        case APP_ID_EXPLORER:        return ICON_EXPLORER;
        case APP_ID_NOTES:           return ICON_FILE;
        case APP_ID_CALCULATOR:      return ICON_CALCULATOR;
        case APP_ID_TERMINAL:        return ICON_TERMINAL;
        case APP_ID_SETTINGS:        return ICON_SETTINGS;
        case APP_ID_MUSIC:           return ICON_MUSIC;
        case APP_ID_ATRIX:           return ICON_ATRIX;
        case APP_ID_TMH:             return ICON_TMH;
        case APP_ID_CONTROLPANEL:    return ICON_SETTINGS;
        case APP_ID_DOOM:            return ICON_DOOM;
        case APP_ID_GRAPH_3D:        return ICON_GRAPH_3D;
        case APP_ID_STRESS_TEST:     return ICON_STRESS_TEST;
        case APP_ID_INPUT_LAB:       return ICON_INPUT_LAB;
        case APP_ID_IMAGE_VIEWER:    return ICON_FILE;
        case APP_ID_SANDBOX:         return ICON_FILE;
        case APP_ID_MINIMAL_BROWSER: return ICON_ATRIX;
        default:                     return ICON_FILE;
    }
}

static StartMenuCategory get_category_for_app_id(uint32_t app_id) {
    switch (app_id) {
        case APP_ID_EXPLORER:
        case APP_ID_NOTES:
        case APP_ID_IMAGE_VIEWER:
            return START_CAT_FILES;
        case APP_ID_TERMINAL:
        case APP_ID_INPUT_LAB:
        case APP_ID_STRESS_TEST:
        case APP_ID_SANDBOX:
            return START_CAT_DEVELOPMENT;
        case APP_ID_ATRIX:
        case APP_ID_MINIMAL_BROWSER:
            return START_CAT_INTERNET;
        case APP_ID_SETTINGS:
        case APP_ID_CALCULATOR:
        case APP_ID_TMH:
        case APP_ID_CONTROLPANEL:
            return START_CAT_SYSTEM;
        case APP_ID_MUSIC:
        case APP_ID_DOOM:
        case APP_ID_GRAPH_3D:
            return START_CAT_MEDIA;
        default:
            return START_CAT_SYSTEM;
    }
}

static const char* get_category_label(StartMenuCategory cat) {
    switch (cat) {
        case START_CAT_FILES:       return "Storage";
        case START_CAT_DEVELOPMENT: return "Tools";
        case START_CAT_INTERNET:    return "Browser";
        case START_CAT_SYSTEM:      return "System";
        case START_CAT_MEDIA:       return "Media";
        default:                    return "App";
    }
}

void StartMenu_RefreshCache(void) {
    uint32_t reg_count = 0;
    HorseAppEntry* reg_apps = horse_get_running(&reg_count);
    if (!reg_apps || reg_count == 0) return;

    s_cached_app_count = 0;
    for (uint32_t i = 0; i < reg_count && s_cached_app_count < 24; i++) {
        uint32_t id = reg_apps[i].app_id;
        if (id == 0) continue;

        s_app_cache[s_cached_app_count].app_id = id;
        s_app_cache[s_cached_app_count].display_name = reg_apps[i].display_name;
        s_app_cache[s_cached_app_count].asset_id = get_asset_for_app_id(id);
        s_app_cache[s_cached_app_count].category = get_category_for_app_id(id);
        s_app_cache[s_cached_app_count].category_label = get_category_label(s_app_cache[s_cached_app_count].category);
        s_app_cache[s_cached_app_count].is_pinned = (id == APP_ID_EXPLORER || id == APP_ID_TERMINAL ||
                                                     id == APP_ID_SETTINGS || id == APP_ID_ATRIX ||
                                                     id == APP_ID_NOTES    || id == APP_ID_CALCULATOR ||
                                                     id == APP_ID_MUSIC    || id == APP_ID_TMH);
        s_cached_app_count++;
    }
}

/* ========================================================================= */
/* Zero-Allocation Substring Matching                                        */
/* ========================================================================= */

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

/* ========================================================================= */
/* High-Performance Clipped UI Rendering Helpers                            */
/* ========================================================================= */

static inline void plot_pixel(const BVFramebuffer* fb, int32_t x, int32_t y, uint32_t color, const BWE_Rect* clip) {
    if (x >= clip->x && x < clip->x + clip->width && y >= clip->y && y < clip->y + clip->height) {
        if (x >= 0 && x < (int32_t)fb->width && y >= 0 && y < (int32_t)fb->height) {
            uint32_t alpha = (color >> 24) & 0xFF;
            uint32_t pitch_pixels = fb->pitch / 4;
            if (pitch_pixels == 0) pitch_pixels = fb->width;

            if (alpha == 0xFF) {
                fb->buffer[y * pitch_pixels + x] = color;
            } else if (alpha > 0) {
                uint32_t dst = fb->buffer[y * pitch_pixels + x];
                uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
                uint32_t sr = (color >> 16) & 0xFF, sg = (color >> 8) & 0xFF, sb = color & 0xFF;
                uint32_t r = (sr * alpha + dr * (255 - alpha)) / 255;
                uint32_t g = (sg * alpha + dg * (255 - alpha)) / 255;
                uint32_t b = (sb * alpha + db * (255 - alpha)) / 255;
                fb->buffer[y * pitch_pixels + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
}

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

/* ========================================================================= */
/* Procedural Vector Icons for High-DPI UI                                   */
/* ========================================================================= */

static void draw_search_glyph(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    float cx = (float)ox + 7.0f;
    float cy = (float)oy + 7.0f;
    for (int32_t y = oy; y <= oy + 15; y++) {
        for (int32_t x = ox; x <= ox + 15; x++) {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float r_sq = dx*dx + dy*dy;
            if (r_sq >= 4.0f * 4.0f && r_sq <= 5.8f * 5.8f) {
                plot_pixel(fb, x, y, color, clip);
            }
            if (x >= ox + 10 && y >= oy + 10 && (x - (ox + 10)) == (y - (oy + 10)) && x <= ox + 14) {
                plot_pixel(fb, x, y, color, clip);
                plot_pixel(fb, x + 1, y, color, clip);
            }
        }
    }
}

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

static void draw_settings_icon_proc(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    float cx = (float)ox + 9.5f;
    float cy = (float)oy + 9.5f;
    for (int32_t y = oy + 2; y <= oy + 17; y++) {
        for (int32_t x = ox + 2; x <= ox + 17; x++) {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float r_sq = dx*dx + dy*dy;
            if (r_sq >= 3.0f * 3.0f && r_sq <= 6.5f * 6.5f) {
                plot_pixel(fb, x, y, color, clip);
            }
        }
    }
    /* 4 gear teeth */
    for (int i = 0; i < 4; i++) {
        plot_pixel(fb, ox + 9, oy + 2, color, clip);
        plot_pixel(fb, ox + 10, oy + 2, color, clip);
        plot_pixel(fb, ox + 9, oy + 17, color, clip);
        plot_pixel(fb, ox + 10, oy + 17, color, clip);
        plot_pixel(fb, ox + 2, oy + 9, color, clip);
        plot_pixel(fb, ox + 2, oy + 10, color, clip);
        plot_pixel(fb, ox + 17, oy + 9, color, clip);
        plot_pixel(fb, ox + 17, oy + 10, color, clip);
    }
}

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

        BWE_DrawTextRole(fb, items[i], rx + 42, item_y + 7, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);
    }
}

/* ========================================================================= */
/* Authoritative Start Menu Main Render Callback                             */
/* ========================================================================= */

static void start_menu_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    int32_t abs_px = self->screen_bounds.x;
    int32_t abs_py = self->screen_bounds.y;
    int32_t panel_w = self->screen_bounds.width;
    int32_t panel_h = self->screen_bounds.height;

    uint32_t bg_col     = 0xF50B1120; // Deep Navy Glassmorphism
    uint32_t border_col = 0xFF334155; // Slate-700 Border

    // 1. Outer Panel Surface
    draw_rounded_panel(fb, abs_px, abs_py, panel_w, panel_h, 16, bg_col, border_col, &clip);

    // 2. Universal Search Header
    int32_t search_x = abs_px + 20;
    int32_t search_y = abs_py + 16;
    int32_t search_w = panel_w - 40;
    int32_t search_h = 42;
    bool search_focused = (s_hover_index == 100);

    uint32_t s_bg = search_focused ? 0xFF1E293B : 0xFF141E33;
    uint32_t s_bd = search_focused ? 0xFF38BDF8 : 0xFF334155;

    draw_rounded_box(fb, search_x, search_y, search_w, search_h, 10, s_bg, &clip);
    for (int32_t y = search_y; y < search_y + search_h; y++) {
        for (int32_t x = search_x; x < search_x + search_w; x++) {
            if (is_outside_rounded_rect(x, y, search_x, search_y, search_w, search_h, 10)) continue;
            bool is_edge = (x == search_x || x == search_x + search_w - 1 || y == search_y || y == search_y + search_h - 1 ||
                            is_outside_rounded_rect(x - 1, y, search_x, search_y, search_w, search_h, 10) ||
                            is_outside_rounded_rect(x + 1, y, search_x, search_y, search_w, search_h, 10) ||
                            is_outside_rounded_rect(x, y - 1, search_x, search_y, search_w, search_h, 10) ||
                            is_outside_rounded_rect(x, y + 1, search_x, search_y, search_w, search_h, 10));
            if (is_edge) plot_pixel(fb, x, y, s_bd, &clip);
        }
    }

    draw_search_glyph(fb, search_x + 14, search_y + 13, 0xFF94A3B8, &clip);

    if (s_search_query[0] != '\0') {
        char disp_text[64];
        strncpy(disp_text, s_search_query, 58);
        disp_text[58] = '\0';
        int len = strlen(disp_text);
        if (search_focused) {
            disp_text[len] = '|';
            disp_text[len + 1] = '\0';
        }
        BWE_DrawTextRole(fb, disp_text, search_x + 40, search_y + 12, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);
    } else {
        const char* ph = search_focused ? "|" : "Search apps, settings and documents...";
        BWE_DrawTextRole(fb, ph, search_x + 40, search_y + 12, 0xFF64748B, BOFONT_ROLE_UI_REGULAR);
    }

    // 3. Middle Content Area: Left Sidebar (Categories) + Right Area (Apps)
    int32_t content_y = search_y + search_h + 16;
    int32_t content_h = panel_h - (content_y - abs_py) - 58;

    /* Left Sidebar: Categories Navigation */
    int32_t nav_x = abs_px + 20;
    int32_t nav_w = 168;

    for (int i = 0; i < START_CAT_COUNT; i++) {
        int32_t cat_y = content_y + i * 42;
        bool is_active = (s_active_category == s_categories[i].category && s_search_query[0] == '\0');
        bool is_hover = (s_hover_index == (110 + i));

        if (is_active) {
            draw_rounded_box(fb, nav_x, cat_y, nav_w, 36, 8, 0x3338BDF8, &clip);
            /* Left Accent Pill Indicator */
            for (int32_t y = cat_y + 6; y <= cat_y + 30; y++) {
                plot_pixel(fb, nav_x + 3, y, 0xFF38BDF8, &clip);
                plot_pixel(fb, nav_x + 4, y, 0xFF38BDF8, &clip);
            }
            BWE_DrawTextRole(fb, s_categories[i].name, nav_x + 16, cat_y + 8, 0xFF38BDF8, BOFONT_ROLE_UI_BOLD);
        } else if (is_hover) {
            draw_rounded_box(fb, nav_x, cat_y, nav_w, 36, 8, 0x1AFFFFFF, &clip);
            BWE_DrawTextRole(fb, s_categories[i].name, nav_x + 16, cat_y + 8, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);
        } else {
            BWE_DrawTextRole(fb, s_categories[i].name, nav_x + 16, cat_y + 8, 0xFF94A3B8, BOFONT_ROLE_UI_REGULAR);
        }
    }

    /* Vertical Divider */
    draw_separator_line(fb, abs_px + 198, content_y, abs_px + 198, content_y + content_h - 10, 0x26334155, &clip);

    /* Right Section: Apps Grid */
    int32_t grid_x = abs_px + 210;
    int32_t grid_w = panel_w - 230;

    /* Section Header */
    if (s_search_query[0] != '\0') {
        BWE_DrawTextRole(fb, "Search Results", grid_x, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
    } else if (s_active_category == START_CAT_ALL) {
        BWE_DrawTextRole(fb, "Pinned Applications", grid_x, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
    } else {
        BWE_DrawTextRole(fb, s_categories[s_active_category].name, grid_x, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
    }

    if (s_cached_app_count == 0) {
        StartMenu_RefreshCache();
    }

    int32_t grid_start_y = content_y + 24;
    int32_t card_w = 136;
    int32_t card_h = 76;
    int32_t cols = 3;
    int32_t spacing_x = 10;
    int32_t spacing_y = 10;

    uint32_t visible_count = 0;
    for (uint32_t i = 0; i < s_cached_app_count && visible_count < 9; i++) {
        /* Filter by search query if active */
        if (s_search_query[0] != '\0') {
            if (!contains_str_nocase(s_app_cache[i].display_name, s_search_query) &&
                !contains_str_nocase(s_app_cache[i].category_label, s_search_query)) {
                continue;
            }
        } else if (s_active_category != START_CAT_ALL) {
            /* Filter by selected category */
            if (s_app_cache[i].category != s_active_category) {
                continue;
            }
        } else {
            /* On 'All', prioritize pinned apps */
            if (!s_app_cache[i].is_pinned && visible_count >= 6) {
                // allow remaining
            }
        }

        int32_t col = visible_count % cols;
        int32_t row = visible_count / cols;
        int32_t cx = grid_x + col * (card_w + spacing_x);
        int32_t cy = grid_start_y + row * (card_h + spacing_y);

        bool is_card_hovered = (s_hover_index == (int32_t)visible_count);

        if (is_card_hovered) {
            draw_rounded_box(fb, cx, cy, card_w, card_h, 10, 0x3338BDF8, &clip);
            /* Border highlight */
            for (int32_t by = cy; by < cy + card_h; by++) {
                for (int32_t bx = cx; bx < cx + card_w; bx++) {
                    if (is_outside_rounded_rect(bx, by, cx, cy, card_w, card_h, 10)) continue;
                    bool is_edge = (bx == cx || bx == cx + card_w - 1 || by == cy || by == cy + card_h - 1 ||
                                    is_outside_rounded_rect(bx - 1, by, cx, cy, card_w, card_h, 10) ||
                                    is_outside_rounded_rect(bx + 1, by, cx, cy, card_w, card_h, 10) ||
                                    is_outside_rounded_rect(bx, by - 1, cx, cy, card_w, card_h, 10) ||
                                    is_outside_rounded_rect(bx, by + 1, cx, cy, card_w, card_h, 10));
                    if (is_edge) plot_pixel(fb, bx, by, 0x8038BDF8, &clip);
                }
            }
        } else {
            draw_rounded_box(fb, cx, cy, card_w, card_h, 10, 0x1A1E293B, &clip);
        }

        /* 32x32 App Icon */
        int32_t ix = cx + (card_w - 32) / 2;
        int32_t iy = cy + 10;
        if (!BOAsset_DrawAsset(s_app_cache[i].asset_id, ix, iy, 32, 32)) {
            BWE_FillRect(fb, ix, iy, 32, 32, 0xFF3B82F6);
        }

        /* App Title */
        BOTextMetrics tm = BOFont_MeasureTextRole(BOFONT_ROLE_UI_MEDIUM, s_app_cache[i].display_name);
        int32_t text_x = cx + (card_w - tm.width) / 2;
        if (text_x < cx + 4) text_x = cx + 4;
        BWE_DrawTextRole(fb, s_app_cache[i].display_name, text_x, cy + 48, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);

        visible_count++;
    }

    if (visible_count == 0) {
        BWE_DrawTextRole(fb, "No matching applications found.", grid_x, grid_start_y + 30, 0xFF64748B, BOFONT_ROLE_UI_REGULAR);
    }

    // 4. Bottom Footer: User Profile + System Actions
    int32_t footer_y = abs_py + panel_h - 52;
    draw_separator_line(fb, abs_px + 16, footer_y, abs_px + panel_w - 16, footer_y, 0x26334155, &clip);

    /* User Profile Pill */
    int32_t av_x = abs_px + 20;
    int32_t av_y = footer_y + 8;
    draw_rounded_box(fb, av_x, av_y, 34, 34, 17, 0xFF2563EB, &clip);
    BWE_DrawTextRole(fb, "S", av_x + 12, av_y + 8, 0xFFFFFFFF, BOFONT_ROLE_UI_BOLD);
    BWE_DrawTextRole(fb, "Saumya", av_x + 44, av_y + 4, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);
    BWE_DrawTextRole(fb, "Administrator", av_x + 44, av_y + 19, 0xFF64748B, BOFONT_ROLE_UI_REGULAR);

    /* Quick Settings Button */
    int32_t set_x = abs_px + panel_w - 96;
    int32_t set_y = footer_y + 8;
    bool set_hover = (s_hover_index == 150);
    if (set_hover) {
        draw_rounded_box(fb, set_x, set_y, 34, 34, 8, 0x33FFFFFF, &clip);
    }
    draw_settings_icon_proc(fb, set_x + 7, set_y + 7, 0xFF94A3B8, &clip);

    /* Power Button */
    int32_t pwr_x = abs_px + panel_w - 54;
    int32_t pwr_y = footer_y + 8;
    bool pwr_hover = (s_hover_index == 151);

    if (pwr_hover || s_power_flyout_open) {
        draw_rounded_box(fb, pwr_x, pwr_y, 34, 34, 8, s_power_flyout_open ? 0x66EF4444 : 0x33FFFFFF, &clip);
    }
    draw_power_icon(fb, pwr_x + 7, pwr_y + 7, 0xFFEF4444, &clip);

    // 5. Power Flyout Popup
    if (s_power_flyout_open) {
        int32_t flyout_w = 160;
        int32_t flyout_h = 120;
        int32_t flyout_x = abs_px + panel_w - 180;
        int32_t flyout_y = footer_y - flyout_h - 8;

        int32_t flyout_hover = (s_hover_index >= 200 && s_hover_index <= 202) ? (s_hover_index - 200) : -1;
        draw_power_flyout(fb, flyout_x, flyout_y, flyout_w, flyout_h, flyout_hover, &clip);
    }
}

/* ========================================================================= */
/* Authoritative Start Menu Event Callback                                   */
/* ========================================================================= */

static void start_menu_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    int32_t abs_px = self->screen_bounds.x;
    int32_t abs_py = self->screen_bounds.y;
    int32_t panel_w = self->screen_bounds.width;
    int32_t panel_h = self->screen_bounds.height;
    int32_t footer_y = abs_py + panel_h - 52;
    int32_t content_y = abs_py + 74;

    /* 1. Mouse Motion & Hover State Routing */
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
            /* Search Bar */
            if (mx >= abs_px + 20 && mx < abs_px + panel_w - 20 && my >= abs_py + 16 && my < abs_py + 58) {
                new_hover = 100;
            }
            /* Settings Button */
            else if (mx >= abs_px + panel_w - 96 && mx < abs_px + panel_w - 62 && my >= footer_y + 8 && my < footer_y + 44) {
                new_hover = 150;
            }
            /* Power Button */
            else if (mx >= abs_px + panel_w - 54 && mx < abs_px + panel_w - 20 && my >= footer_y + 8 && my < footer_y + 44) {
                new_hover = 151;
            }
            /* Left Sidebar Categories */
            else if (mx >= abs_px + 20 && mx < abs_px + 188 && my >= content_y && my < content_y + START_CAT_COUNT * 42) {
                int cat_idx = (my - content_y) / 42;
                if (cat_idx >= 0 && cat_idx < START_CAT_COUNT) {
                    new_hover = 110 + cat_idx;
                }
            }
            /* Right Grid App Cards */
            else {
                int32_t grid_x = abs_px + 210;
                int32_t grid_start_y = content_y + 24;
                int32_t card_w = 136;
                int32_t card_h = 76;
                int32_t cols = 3;
                int32_t spacing_x = 10;
                int32_t spacing_y = 10;

                uint32_t visible_count = 0;
                for (uint32_t i = 0; i < s_cached_app_count && visible_count < 9; i++) {
                    if (s_search_query[0] != '\0') {
                        if (!contains_str_nocase(s_app_cache[i].display_name, s_search_query) &&
                            !contains_str_nocase(s_app_cache[i].category_label, s_search_query)) {
                            continue;
                        }
                    } else if (s_active_category != START_CAT_ALL) {
                        if (s_app_cache[i].category != s_active_category) {
                            continue;
                        }
                    }

                    int32_t col = visible_count % cols;
                    int32_t row = visible_count / cols;
                    int32_t cx = grid_x + col * (card_w + spacing_x);
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

    /* 2. Keyboard Event Routing (Live Search, Escape, Enter) */
    if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t key = event->data.key.key_code;
        uint32_t ch  = event->data.key.character;

        if (key == 27) { // Escape key closes menu
            StartMenu_Close();
            return;
        }

        if (key == 13 || key == 10) { // Enter key launches first matched app
            uint32_t target_app_id = 0;
            for (uint32_t i = 0; i < s_cached_app_count; i++) {
                if (s_search_query[0] != '\0') {
                    if (contains_str_nocase(s_app_cache[i].display_name, s_search_query) ||
                        contains_str_nocase(s_app_cache[i].category_label, s_search_query)) {
                        target_app_id = s_app_cache[i].app_id;
                        break;
                    }
                } else if (s_active_category != START_CAT_ALL) {
                    if (s_app_cache[i].category == s_active_category) {
                        target_app_id = s_app_cache[i].app_id;
                        break;
                    }
                } else {
                    target_app_id = s_app_cache[0].app_id;
                    break;
                }
            }
            if (target_app_id != 0) {
                StartMenu_Close();
                horse_launch(target_app_id);
            }
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

        if (ch >= 32 && ch <= 126) { // Printable ASCII typing
            int len = strlen(s_search_query);
            if (len < 50) {
                s_search_query[len] = (char)ch;
                s_search_query[len + 1] = '\0';
                BWE_InvalidateWindow(window_id);
            }
            return;
        }
    }

    /* 3. Mouse Click Handling */
    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        /* Click outside panel dismisses Start Menu */
        if (mx < abs_px || mx >= abs_px + panel_w || my < abs_py || my >= abs_py + panel_h) {
            StartMenu_Close();
            return;
        }

        /* Power Flyout Handling */
        if (s_power_flyout_open) {
            int32_t flyout_w = 160;
            int32_t flyout_h = 120;
            int32_t flyout_x = abs_px + panel_w - 180;
            int32_t flyout_y = footer_y - flyout_h - 8;

            if (mx >= flyout_x && mx < flyout_x + flyout_w && my >= flyout_y && my < flyout_y + flyout_h) {
                int item = (my - (flyout_y + 8)) / 36;
                StartMenu_Close();

                if (item == 1) {
                    horse_restart();
                } else if (item == 2) {
                    horse_shutdown();
                }
                return;
            } else {
                s_power_flyout_open = false;
                BWE_InvalidateWindow(window_id);
            }
        }

        /* Power Action Button Click */
        if (mx >= abs_px + panel_w - 54 && mx < abs_px + panel_w - 20 && my >= footer_y + 8 && my < footer_y + 44) {
            s_power_flyout_open = !s_power_flyout_open;
            BWE_InvalidateWindow(window_id);
            return;
        }

        /* Quick Settings Shortcut Click */
        if (mx >= abs_px + panel_w - 96 && mx < abs_px + panel_w - 62 && my >= footer_y + 8 && my < footer_y + 44) {
            StartMenu_Close();
            horse_launch(APP_ID_SETTINGS);
            return;
        }

        /* Left Sidebar Category Navigation Click */
        if (mx >= abs_px + 20 && mx < abs_px + 188 && my >= content_y && my < content_y + START_CAT_COUNT * 42) {
            int cat_idx = (my - content_y) / 42;
            if (cat_idx >= 0 && cat_idx < START_CAT_COUNT) {
                s_active_category = s_categories[cat_idx].category;
                s_search_query[0] = '\0';
                BWE_InvalidateWindow(window_id);
                return;
            }
        }

        /* Right Grid App Card Click */
        int32_t grid_x = abs_px + 210;
        int32_t grid_start_y = content_y + 24;
        int32_t card_w = 136;
        int32_t card_h = 76;
        int32_t cols = 3;
        int32_t spacing_x = 10;
        int32_t spacing_y = 10;

        uint32_t visible_count = 0;
        for (uint32_t i = 0; i < s_cached_app_count && visible_count < 9; i++) {
            if (s_search_query[0] != '\0') {
                if (!contains_str_nocase(s_app_cache[i].display_name, s_search_query) &&
                    !contains_str_nocase(s_app_cache[i].category_label, s_search_query)) {
                    continue;
                }
            } else if (s_active_category != START_CAT_ALL) {
                if (s_app_cache[i].category != s_active_category) {
                    continue;
                }
            }

            int32_t col = visible_count % cols;
            int32_t row = visible_count / cols;
            int32_t cx = grid_x + col * (card_w + spacing_x);
            int32_t cy = grid_start_y + row * (card_h + spacing_y);

            if (mx >= cx && mx < cx + card_w && my >= cy && my < cy + card_h) {
                uint32_t target_app_id = s_app_cache[i].app_id;
                StartMenu_Close();
                horse_launch(target_app_id);
                return;
            }
            visible_count++;
        }
    }
}

/* ========================================================================= */
/* Start Menu Control & Lifecycle APIs                                       */
/* ========================================================================= */

void StartMenu_Open(void) {
    if (!g_start_menu_win_id) return;
    StartMenu_RefreshCache();
    s_search_query[0] = '\0';
    s_active_category = START_CAT_ALL;
    s_power_flyout_open = false;
    s_hover_index = -1;
    g_start_menu_open = true;

    BOS_Show(g_start_menu_win_id);
    BOS_SetFocus(g_start_menu_win_id);
    BWE_InvalidateWindow(g_start_menu_win_id);

    extern uint32_t g_task_panel_win_id;
    if (g_task_panel_win_id) BWE_InvalidateWindow(g_task_panel_win_id);
}

void StartMenu_Close(void) {
    if (!g_start_menu_win_id) return;
    s_power_flyout_open = false;
    s_search_query[0] = '\0';
    s_hover_index = -1;
    g_start_menu_open = false;

    BOS_Hide(g_start_menu_win_id);

    extern uint32_t g_task_panel_win_id;
    if (g_task_panel_win_id) BWE_InvalidateWindow(g_task_panel_win_id);
}

void StartMenu_Toggle(void) {
    if (g_start_menu_open) {
        StartMenu_Close();
    } else {
        StartMenu_Open();
    }
}

void StartMenu_Initialize(void) {
    const AGDAE_Metrics* metrics = AGDAE_GetMetrics();
    int32_t sw = metrics->desktop_rect.width;
    int32_t sh = metrics->desktop_rect.height;
    
    int32_t panel_w = 660;
    int32_t panel_h = 490;
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
