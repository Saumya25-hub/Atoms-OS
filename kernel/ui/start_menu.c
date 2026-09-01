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

static StartMenu_Layout s_sm_layout;

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

/* ========================================================================= */
/* Dedicated Offscreen Backing Surface (Zero Intermediate In-Place Mutation) */
/* ========================================================================= */
static uint32_t s_sm_backing_surface[640 * 460] __attribute__((aligned(16)));
static bool     s_sm_backing_valid = false;

static void start_menu_compute_layout(int32_t screen_w, int32_t screen_h) {
    if (screen_w <= 0) screen_w = 1024;
    if (screen_h <= 0) screen_h = 768;

    int32_t panel_w = 640;
    int32_t panel_h = 460;
    if (panel_w > screen_w - 40) panel_w = screen_w - 40;
    if (panel_h > screen_h - 80) panel_h = screen_h - 80;

    int32_t px = (screen_w - panel_w) / 2;
    int32_t py = screen_h - 64 - panel_h - 10;
    if (py < 10) py = 10;

    s_sm_layout.x = px;
    s_sm_layout.y = py;
    s_sm_layout.width = panel_w;
    s_sm_layout.height = panel_h;

    s_sm_layout.search_x = px + 20;
    s_sm_layout.search_y = py + 16;
    s_sm_layout.search_w = panel_w - 40;
    s_sm_layout.search_h = 42;

    int32_t content_y = s_sm_layout.search_y + s_sm_layout.search_h + 16;
    s_sm_layout.nav_x = px + 20;
    s_sm_layout.nav_y = content_y;
    s_sm_layout.nav_w = 168;

    s_sm_layout.grid_x = px + 210;
    s_sm_layout.grid_y = content_y;
    s_sm_layout.grid_w = panel_w - 230;

    s_sm_layout.footer_y = py + panel_h - 52;
}

const StartMenu_Layout* StartMenu_GetLayout(void) {
    return &s_sm_layout;
}

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
    s_sm_backing_valid = false;
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
        }
    }
    for (int32_t i = 0; i < 6; i++) {
        plot_pixel(fb, ox + 11 + i, oy + 11 + i, color, clip);
        plot_pixel(fb, ox + 12 + i, oy + 11 + i, color, clip);
    }
}

static void draw_settings_icon_proc(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    float cx = (float)ox + 8.5f;
    float cy = (float)oy + 8.5f;
    for (int32_t y = oy; y <= oy + 17; y++) {
        for (int32_t x = ox; x <= ox + 17; x++) {
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float r_sq = dx*dx + dy*dy;
            if (r_sq >= 3.5f * 3.5f && r_sq <= 5.2f * 5.2f) {
                plot_pixel(fb, x, y, color, clip);
            }
        }
    }
    for (int32_t i = 0; i < 4; i++) {
        plot_pixel(fb, ox + 7 + i, oy + 1, color, clip);
        plot_pixel(fb, ox + 7 + i, oy + 16, color, clip);
        plot_pixel(fb, ox + 1, oy + 7 + i, color, clip);
        plot_pixel(fb, ox + 16, oy + 7 + i, color, clip);
    }
}

static void draw_power_icon(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    float cx = (float)ox + 8.5f;
    float cy = (float)oy + 9.5f;
    for (int32_t y = oy + 2; y <= oy + 17; y++) {
        for (int32_t x = ox; x <= ox + 17; x++) {
            if (x >= ox + 7 && x <= ox + 9 && y <= oy + 7) continue;
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float r_sq = dx*dx + dy*dy;
            if (r_sq >= 5.0f * 5.0f && r_sq <= 7.0f * 7.0f) {
                plot_pixel(fb, x, y, color, clip);
            }
        }
    }
    for (int32_t y = oy + 2; y <= oy + 9; y++) {
        plot_pixel(fb, ox + 8, y, color, clip);
        plot_pixel(fb, ox + 9, y, color, clip);
    }
}

static void draw_restart_icon(const BVFramebuffer* fb, int32_t ox, int32_t oy, uint32_t color, const BWE_Rect* clip) {
    float cx = (float)ox + 8.5f;
    float cy = (float)oy + 8.5f;
    for (int32_t y = oy + 1; y <= oy + 16; y++) {
        for (int32_t x = ox + 1; x <= ox + 16; x++) {
            if (x >= ox + 11 && y <= oy + 6) continue;
            float dx = (float)x - cx;
            float dy = (float)y - cy;
            float r_sq = dx*dx + dy*dy;
            if (r_sq >= 4.5f * 4.5f && r_sq <= 6.5f * 6.5f) {
                plot_pixel(fb, x, y, color, clip);
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
/* Pre-Rasterized Start Menu Backing Surface Construction                    */
/* ========================================================================= */

static void start_menu_build_backing_surface(int32_t panel_w, int32_t panel_h) {
    if (panel_w > 640) panel_w = 640;
    if (panel_h > 460) panel_h = 460;

    BVFramebuffer sm_fb;
    sm_fb.buffer = s_sm_backing_surface;
    sm_fb.width = panel_w;
    sm_fb.height = panel_h;
    sm_fb.pitch = panel_w * 4;

    BWE_Rect clip = {0, 0, panel_w, panel_h};

    /* Clear surface */
    uint32_t total_px = (uint32_t)(panel_w * panel_h);
    for (uint32_t i = 0; i < total_px; i++) {
        s_sm_backing_surface[i] = 0x00000000;
    }

    uint32_t bg_col     = 0xF50B1120; // Deep Navy Glassmorphism
    uint32_t border_col = 0xFF334155; // Slate-700 Border

    // 1. Outer Panel Surface
    draw_rounded_panel(&sm_fb, 0, 0, panel_w, panel_h, 16, bg_col, border_col, &clip);

    // 2. Universal Search Header
    int32_t search_x = 20;
    int32_t search_y = 16;
    int32_t search_w = panel_w - 40;
    int32_t search_h = 42;

    uint32_t s_bg = 0xFF141E33;
    uint32_t s_bd = 0xFF334155;

    draw_rounded_box(&sm_fb, search_x, search_y, search_w, search_h, 10, s_bg, &clip);
    for (int32_t y = search_y; y < search_y + search_h; y++) {
        for (int32_t x = search_x; x < search_x + search_w; x++) {
            if (is_outside_rounded_rect(x, y, search_x, search_y, search_w, search_h, 10)) continue;
            bool is_edge = (x == search_x || x == search_x + search_w - 1 || y == search_y || y == search_y + search_h - 1 ||
                            is_outside_rounded_rect(x - 1, y, search_x, search_y, search_w, search_h, 10) ||
                            is_outside_rounded_rect(x + 1, y, search_x, search_y, search_w, search_h, 10) ||
                            is_outside_rounded_rect(x, y - 1, search_x, search_y, search_w, search_h, 10) ||
                            is_outside_rounded_rect(x, y + 1, search_x, search_y, search_w, search_h, 10));
            if (is_edge) plot_pixel(&sm_fb, x, y, s_bd, &clip);
        }
    }

    draw_search_glyph(&sm_fb, search_x + 14, search_y + 13, 0xFF94A3B8, &clip);

    if (s_search_query[0] != '\0') {
        char disp_text[64];
        strncpy(disp_text, s_search_query, 58);
        disp_text[58] = '\0';
        BWE_DrawTextRole(&sm_fb, disp_text, search_x + 40, search_y + 12, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);
    } else {
        BWE_DrawTextRole(&sm_fb, "Search apps, settings and documents...", search_x + 40, search_y + 12, 0xFF64748B, BOFONT_ROLE_UI_REGULAR);
    }

    // 3. Middle Content Area: Left Sidebar (Categories) + Right Area (Apps)
    int32_t content_y = search_y + search_h + 16;
    int32_t content_h = panel_h - content_y - 58;

    /* Left Sidebar: Categories Navigation */
    int32_t nav_x = 20;
    int32_t nav_w = 168;

    for (int i = 0; i < START_CAT_COUNT; i++) {
        int32_t cat_y = content_y + i * 42;
        bool is_active = (s_active_category == s_categories[i].category && s_search_query[0] == '\0');

        if (is_active) {
            draw_rounded_box(&sm_fb, nav_x, cat_y, nav_w, 36, 8, 0x3338BDF8, &clip);
            /* Left Accent Pill Indicator */
            for (int32_t y = cat_y + 6; y <= cat_y + 30; y++) {
                plot_pixel(&sm_fb, nav_x + 3, y, 0xFF38BDF8, &clip);
                plot_pixel(&sm_fb, nav_x + 4, y, 0xFF38BDF8, &clip);
            }
            BWE_DrawTextRole(&sm_fb, s_categories[i].name, nav_x + 16, cat_y + 8, 0xFF38BDF8, BOFONT_ROLE_UI_BOLD);
        } else {
            BWE_DrawTextRole(&sm_fb, s_categories[i].name, nav_x + 16, cat_y + 8, 0xFF94A3B8, BOFONT_ROLE_UI_REGULAR);
        }
    }

    /* Vertical Divider */
    draw_separator_line(&sm_fb, 198, content_y, 198, content_y + content_h - 10, 0x26334155, &clip);

    /* Right Section: Apps Grid */
    int32_t grid_x = 210;

    /* Section Header */
    if (s_search_query[0] != '\0') {
        BWE_DrawTextRole(&sm_fb, "Search Results", grid_x, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
    } else if (s_active_category == START_CAT_ALL) {
        BWE_DrawTextRole(&sm_fb, "Pinned Applications", grid_x, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
    } else {
        BWE_DrawTextRole(&sm_fb, s_categories[s_active_category].name, grid_x, content_y, 0xFF94A3B8, BOFONT_ROLE_UI_BOLD);
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
        }

        int32_t col = visible_count % cols;
        int32_t row = visible_count / cols;
        int32_t cx = grid_x + col * (card_w + spacing_x);
        int32_t cy = grid_start_y + row * (card_h + spacing_y);

        /* Base Unhovered Card */
        draw_rounded_box(&sm_fb, cx, cy, card_w, card_h, 10, 0x1A1E293B, &clip);

        /* 32x32 App Icon */
        int32_t ix = cx + (card_w - 32) / 2;
        int32_t iy = cy + 10;
        if (!BOAsset_DrawAsset(s_app_cache[i].asset_id, ix, iy, 32, 32)) {
            BWE_FillRect(&sm_fb, ix, iy, 32, 32, 0xFF3B82F6);
        }

        /* App Title */
        BOTextMetrics tm = BOFont_MeasureTextRole(BOFONT_ROLE_UI_MEDIUM, s_app_cache[i].display_name);
        int32_t text_x = cx + (card_w - tm.width) / 2;
        if (text_x < cx + 4) text_x = cx + 4;
        BWE_DrawTextRole(&sm_fb, s_app_cache[i].display_name, text_x, cy + 48, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);

        visible_count++;
    }

    if (visible_count == 0) {
        BWE_DrawTextRole(&sm_fb, "No matching applications found.", grid_x, grid_start_y + 30, 0xFF64748B, BOFONT_ROLE_UI_REGULAR);
    }

    // 4. Bottom Footer: User Profile + System Actions
    int32_t footer_y = panel_h - 52;
    draw_separator_line(&sm_fb, 16, footer_y, panel_w - 16, footer_y, 0x26334155, &clip);

    /* User Profile Pill */
    int32_t av_x = 20;
    int32_t av_y = footer_y + 8;
    draw_rounded_box(&sm_fb, av_x, av_y, 34, 34, 17, 0xFF2563EB, &clip);
    BWE_DrawTextRole(&sm_fb, "S", av_x + 12, av_y + 8, 0xFFFFFFFF, BOFONT_ROLE_UI_BOLD);
    BWE_DrawTextRole(&sm_fb, "Saumya", av_x + 44, av_y + 4, 0xFFF1F5F9, BOFONT_ROLE_UI_MEDIUM);
    BWE_DrawTextRole(&sm_fb, "Administrator", av_x + 44, av_y + 19, 0xFF64748B, BOFONT_ROLE_UI_REGULAR);

    /* Quick Settings Button */
    int32_t set_x = panel_w - 96;
    int32_t set_y = footer_y + 8;
    draw_rounded_box(&sm_fb, set_x, set_y, 34, 34, 17, 0x1AFFFFFF, &clip);
    draw_settings_icon_proc(&sm_fb, set_x + 8, set_y + 8, 0xFF94A3B8, &clip);

    /* Power Action Button */
    int32_t pwr_x = panel_w - 54;
    int32_t pwr_y = footer_y + 8;
    draw_rounded_box(&sm_fb, pwr_x, pwr_y, 34, 34, 17, 0x1AFFFFFF, &clip);
    draw_power_icon(&sm_fb, pwr_x + 8, pwr_y + 8, 0xFFEF4444, &clip);

    s_sm_backing_valid = true;
}

/* ========================================================================= */
/* Scoped Damage Bounding Box Calculation                                    */
/* ========================================================================= */

static bool start_menu_get_element_bounds(int32_t hover_idx, BWE_Rect* out_rect) {
    if (!out_rect) return false;

    int32_t px = s_sm_layout.x;
    int32_t py = s_sm_layout.y;
    int32_t panel_w = s_sm_layout.width;
    int32_t content_y = s_sm_layout.nav_y;
    int32_t footer_y = s_sm_layout.footer_y;

    if (hover_idx >= 0 && hover_idx < 9) {
        int32_t grid_x = s_sm_layout.grid_x;
        int32_t grid_start_y = content_y + 24;
        int32_t card_w = 136;
        int32_t card_h = 76;
        int32_t cols = 3;
        int32_t spacing_x = 10;
        int32_t spacing_y = 10;

        int32_t col = hover_idx % cols;
        int32_t row = hover_idx / cols;
        out_rect->x = grid_x + col * (card_w + spacing_x);
        out_rect->y = grid_start_y + row * (card_h + spacing_y);
        out_rect->width = card_w;
        out_rect->height = card_h;
        return true;
    }

    if (hover_idx == 100) {
        out_rect->x = s_sm_layout.search_x;
        out_rect->y = s_sm_layout.search_y;
        out_rect->width = s_sm_layout.search_w;
        out_rect->height = s_sm_layout.search_h;
        return true;
    }

    if (hover_idx >= 110 && hover_idx < 110 + START_CAT_COUNT) {
        int32_t cat_idx = hover_idx - 110;
        out_rect->x = s_sm_layout.nav_x;
        out_rect->y = content_y + cat_idx * 42;
        out_rect->width = s_sm_layout.nav_w;
        out_rect->height = 36;
        return true;
    }

    if (hover_idx == 150) {
        out_rect->x = px + panel_w - 96;
        out_rect->y = footer_y + 8;
        out_rect->width = 34;
        out_rect->height = 34;
        return true;
    }

    if (hover_idx == 151) {
        out_rect->x = px + panel_w - 54;
        out_rect->y = footer_y + 8;
        out_rect->width = 34;
        out_rect->height = 34;
        return true;
    }

    if (hover_idx >= 200 && hover_idx <= 202) {
        out_rect->x = px + panel_w - 180;
        out_rect->y = footer_y - 120 - 8;
        out_rect->width = 160;
        out_rect->height = 120;
        return true;
    }

    return false;
}

/* ========================================================================= */
/* Authoritative Start Menu Main Render Callback                             */
/* ========================================================================= */

static void start_menu_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb || !fb->buffer) return;

    start_menu_compute_layout((int32_t)fb->width, (int32_t)fb->height);

    int32_t abs_px = s_sm_layout.x;
    int32_t abs_py = s_sm_layout.y;
    int32_t panel_w = s_sm_layout.width;
    int32_t panel_h = s_sm_layout.height;

    self->screen_bounds.x = abs_px;
    self->screen_bounds.y = abs_py;
    self->screen_bounds.width = panel_w;
    self->screen_bounds.height = panel_h;

    if (!s_sm_backing_valid) {
        start_menu_build_backing_surface(panel_w, panel_h);
    }

    uint32_t fb_pitch_pixels = fb->pitch / 4;
    if (fb_pitch_pixels == 0) fb_pitch_pixels = fb->width;

    /* 1. Fast, Atomic Scanline Blit from Backing Surface into RAM Target */
    for (int32_t y = 0; y < panel_h; y++) {
        int32_t screen_y = abs_py + y;
        if (screen_y < 0 || screen_y >= (int32_t)fb->height) continue;

        uint32_t ram_row_idx = (uint32_t)screen_y * fb_pitch_pixels + (uint32_t)abs_px;
        uint32_t src_row_idx = (uint32_t)y * (uint32_t)panel_w;

        for (int32_t x = 0; x < panel_w; x++) {
            int32_t screen_x = abs_px + x;
            if (screen_x < 0 || screen_x >= (int32_t)fb->width) continue;

            uint32_t pixel = s_sm_backing_surface[src_row_idx + x];
            if (pixel == 0) continue; // Transparent rounded corner

            fb->buffer[ram_row_idx + x] = pixel;
        }
    }

    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    /* 2. Atomic Hover Dynamic Highlight Overlay */
    if (s_hover_index >= 0 && s_hover_index < 9) {
        BWE_Rect card_rect;
        if (start_menu_get_element_bounds(s_hover_index, &card_rect)) {
            for (int32_t by = card_rect.y; by < card_rect.y + card_rect.height; by++) {
                for (int32_t bx = card_rect.x; bx < card_rect.x + card_rect.width; bx++) {
                    if (is_outside_rounded_rect(bx, by, card_rect.x, card_rect.y, card_rect.width, card_rect.height, 10)) continue;
                    bool is_edge = (bx == card_rect.x || bx == card_rect.x + card_rect.width - 1 || 
                                    by == card_rect.y || by == card_rect.y + card_rect.height - 1 ||
                                    is_outside_rounded_rect(bx - 1, by, card_rect.x, card_rect.y, card_rect.width, card_rect.height, 10) ||
                                    is_outside_rounded_rect(bx + 1, by, card_rect.x, card_rect.y, card_rect.width, card_rect.height, 10) ||
                                    is_outside_rounded_rect(bx, by - 1, card_rect.x, card_rect.y, card_rect.width, card_rect.height, 10) ||
                                    is_outside_rounded_rect(bx, by + 1, card_rect.x, card_rect.y, card_rect.width, card_rect.height, 10));
                    if (is_edge) {
                        plot_pixel(fb, bx, by, 0x8038BDF8, &clip);
                    } else {
                        plot_pixel(fb, bx, by, 0x2238BDF8, &clip);
                    }
                }
            }
        }
    } else if (s_hover_index >= 110 && s_hover_index < 110 + START_CAT_COUNT) {
        BWE_Rect cat_rect;
        if (start_menu_get_element_bounds(s_hover_index, &cat_rect)) {
            draw_rounded_box(fb, cat_rect.x, cat_rect.y, cat_rect.width, cat_rect.height, 8, 0x1AFFFFFF, &clip);
        }
    } else if (s_hover_index == 150) {
        BWE_Rect set_rect;
        if (start_menu_get_element_bounds(150, &set_rect)) {
            draw_rounded_box(fb, set_rect.x, set_rect.y, set_rect.width, set_rect.height, 17, 0x3338BDF8, &clip);
            draw_settings_icon_proc(fb, set_rect.x + 8, set_rect.y + 8, 0xFFF1F5F9, &clip);
        }
    } else if (s_hover_index == 151) {
        BWE_Rect pwr_rect;
        if (start_menu_get_element_bounds(151, &pwr_rect)) {
            draw_rounded_box(fb, pwr_rect.x, pwr_rect.y, pwr_rect.width, pwr_rect.height, 17, 0x33EF4444, &clip);
            draw_power_icon(fb, pwr_rect.x + 8, pwr_rect.y + 8, 0xFFFF7777, &clip);
        }
    }

    /* 3. Power Flyout Modal (if opened) */
    if (s_power_flyout_open) {
        int32_t flyout_w = 160;
        int32_t flyout_h = 120;
        int32_t flyout_x = abs_px + panel_w - 180;
        int32_t flyout_y = s_sm_layout.footer_y - flyout_h - 8;
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

    int32_t abs_px = s_sm_layout.x;
    int32_t abs_py = s_sm_layout.y;
    int32_t panel_w = s_sm_layout.width;
    int32_t panel_h = s_sm_layout.height;
    int32_t footer_y = s_sm_layout.footer_y;
    int32_t content_y = s_sm_layout.nav_y;

    /* 1. Mouse Motion & Scoped Hover Routing */
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
                int32_t grid_x = s_sm_layout.grid_x;
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
            int32_t old_hover = s_hover_index;
            s_hover_index = new_hover;

            /* Scoped Invalidation: Only invalidate the changed elements, not entire window */
            BWE_Rect r_old, r_new;
            bool has_old = start_menu_get_element_bounds(old_hover, &r_old);
            bool has_new = start_menu_get_element_bounds(new_hover, &r_new);

            extern void BWE_AddCompositorDirtyRect(const BWE_Rect* rect);
            if (has_old) BWE_AddCompositorDirtyRect(&r_old);
            if (has_new) BWE_AddCompositorDirtyRect(&r_new);

            self->is_dirty = true;
            extern void BCM_RequestWindowDamage(uint32_t window_id);
            BCM_RequestWindowDamage(window_id);
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

        if (key == 8) { // Backspace key
            int len = strlen(s_search_query);
            if (len > 0) {
                s_search_query[len - 1] = '\0';
                s_sm_backing_valid = false;
                BWE_InvalidateWindow(window_id);
            }
            return;
        }

        if (ch >= 32 && ch <= 126) { // Printable ASCII typing
            int len = strlen(s_search_query);
            if (len < 50) {
                s_search_query[len] = (char)ch;
                s_search_query[len + 1] = '\0';
                s_sm_backing_valid = false;
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
                s_sm_backing_valid = false;
                BWE_InvalidateWindow(window_id);
                return;
            }
        }

        /* Right Grid App Card Click */
        int32_t grid_x = s_sm_layout.grid_x;
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
    
    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);

    uint32_t scr_w = BOVISUAL_Graphics_GetWidth();
    uint32_t scr_h = BOVISUAL_Graphics_GetHeight();
    if (scr_w == 0) scr_w = g_kernel_screen_width > 0 ? g_kernel_screen_width : 1024;
    if (scr_h == 0) scr_h = g_kernel_screen_height > 0 ? g_kernel_screen_height : 768;

    start_menu_compute_layout((int32_t)scr_w, (int32_t)scr_h);

    BWE_Window* sm = BWE_GetWindow(g_start_menu_win_id);
    if (sm) {
        sm->screen_bounds.x = s_sm_layout.x;
        sm->screen_bounds.y = s_sm_layout.y;
        sm->screen_bounds.width = s_sm_layout.width;
        sm->screen_bounds.height = s_sm_layout.height;
        sm->local_bounds.x = s_sm_layout.x;
        sm->local_bounds.y = s_sm_layout.y;
        sm->local_bounds.width = s_sm_layout.width;
        sm->local_bounds.height = s_sm_layout.height;
        sm->state = BWE_STATE_SHOWN;
        sm->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST | BWE_WINDOW_TRANSPARENT;
    }

    StartMenu_RefreshCache();
    s_search_query[0] = '\0';
    s_active_category = START_CAT_ALL;
    s_power_flyout_open = false;
    s_hover_index = -1;
    s_sm_backing_valid = false;
    g_start_menu_open = true;

    BOS_Show(g_start_menu_win_id);
    BOS_SetFocus(g_start_menu_win_id);
    BWE_BringToFront(g_start_menu_win_id);
    BWE_UpdateZOrders();

    BWE_Rect sm_rect = { s_sm_layout.x, s_sm_layout.y, s_sm_layout.width, s_sm_layout.height };
    extern void BWE_AddCompositorDirtyRect(const BWE_Rect* rect);
    extern void BCM_RequestWindowDamage(uint32_t window_id);
    BWE_AddCompositorDirtyRect(&sm_rect);
    BCM_RequestWindowDamage(g_start_menu_win_id);
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

    BWE_Rect sm_rect = { s_sm_layout.x, s_sm_layout.y, s_sm_layout.width, s_sm_layout.height };
    extern void BWE_AddCompositorDirtyRect(const BWE_Rect* rect);
    BWE_AddCompositorDirtyRect(&sm_rect);
    extern void desktop_refresh_background(void);
    desktop_refresh_background();

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
    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;
    uint32_t scr_w = g_kernel_screen_width > 0 ? g_kernel_screen_width : 1024;
    uint32_t scr_h = g_kernel_screen_height > 0 ? g_kernel_screen_height : 768;

    start_menu_compute_layout((int32_t)scr_w, (int32_t)scr_h);

    StartMenu_RefreshCache();

    BOS_CreatePanel(BWE_DESKTOP_ID, s_sm_layout.x, s_sm_layout.y, s_sm_layout.width, s_sm_layout.height, 0x00000000, &g_start_menu_win_id);
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
