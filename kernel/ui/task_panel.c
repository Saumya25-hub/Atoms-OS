#include "task_panel.h"
#include "kernel/ui/icon_engine/include/icon_engine.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/drivers/rtc/rtc.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern BWE_Window g_windows[];

uint32_t g_task_panel_win_id = 0;
extern uint32_t g_start_menu_win_id;
extern bool g_start_menu_open;

// String helper
static bool contains_str(const char* haystack, const char* needle) {
    if (!haystack || !needle) return false;
    int hlen = strlen(haystack);
    int nlen = strlen(needle);
    if (nlen > hlen) return false;
    for (int i = 0; i <= hlen - nlen; i++) {
        bool match = true;
        for (int j = 0; j < nlen; j++) {
            if (haystack[i + j] != needle[j]) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}

// Asset resolution helper
static uint32_t get_asset_for_app(uint32_t app_id) {
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
        default:                 return ICON_FILE;
    }
}

typedef struct {
    uint32_t app_id;
    const char* name;
} PinnedAppDef;

// Canonical 10 core ATOMS desktop apps
static const PinnedAppDef s_pinned_apps[] = {
    { APP_ID_EXPLORER,    "Files" },
    { APP_ID_TERMINAL,    "Terminal" },
    { APP_ID_NOTES,       "Notes" },
    { APP_ID_CALCULATOR,  "Calculator" },
    { APP_ID_SETTINGS,    "Settings" },
    { APP_ID_MUSIC,       "Media Player" },
    { APP_ID_TMH,         "Task Manager" },
    { APP_ID_ATRIX,       "ATRIX" },
    { APP_ID_GRAPH_3D,    "3D Benchmark" },
    { APP_ID_DOOM,        "DOOM" }
};
#define NUM_PINNED_APPS (sizeof(s_pinned_apps)/sizeof(s_pinned_apps[0]))

static uint32_t find_window_for_app(uint32_t app_id) {
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED && win->id != 0 && win->parent_id == BWE_DESKTOP_ID && win->type == BWE_TYPE_WINDOW) {
            if (win->id == g_task_panel_win_id || win->id == g_start_menu_win_id) continue;
            uint32_t win_app_id = (uint32_t)(uintptr_t)win->user_data;
            if (win_app_id == app_id) return win->id;
            
            const char* title = (win->title[0] != '\0') ? win->title : win->control_data.button.text;
            if (app_id == APP_ID_EXPLORER && contains_str(title, "Explorer")) return win->id;
            if (app_id == APP_ID_NOTES && contains_str(title, "Notes")) return win->id;
            if (app_id == APP_ID_TERMINAL && contains_str(title, "Terminal")) return win->id;
            if (app_id == APP_ID_SETTINGS && contains_str(title, "Settings")) return win->id;
            if (app_id == APP_ID_CALCULATOR && contains_str(title, "Calc")) return win->id;
            if (app_id == APP_ID_MUSIC && (contains_str(title, "Music") || contains_str(title, "Media"))) return win->id;
            if (app_id == APP_ID_TMH && (contains_str(title, "Task Manager") || contains_str(title, "TMH"))) return win->id;
            if (app_id == APP_ID_ATRIX && contains_str(title, "ATRIX")) return win->id;
            if (app_id == APP_ID_GRAPH_3D && (contains_str(title, "Graph") || contains_str(title, "3D"))) return win->id;
            if (app_id == APP_ID_DOOM && contains_str(title, "DOOM")) return win->id;
        }
    }
    return 0;
}

// Single Authoritative Layout State
static Taskbar_Layout s_layout;
static int32_t s_hover_index = -1; // 0 = Start, 1..N = Apps, 1000 = Tray/Clock

const Taskbar_Layout* TaskPanel_GetLayout(void) {
    return &s_layout;
}

static void task_panel_compute_layout(int32_t screen_w, int32_t screen_h) {
    s_layout.screen_w = screen_w;
    s_layout.screen_h = screen_h;

    // 1. Gather Pinned Apps and Window States
    s_layout.app_count = 0;
    for (size_t i = 0; i < NUM_PINNED_APPS && s_layout.app_count < TASKBAR_MAX_APPS; i++) {
        TaskbarAppSlot* slot = &s_layout.apps[s_layout.app_count];
        slot->app_id = s_pinned_apps[i].app_id;
        slot->asset_id = get_asset_for_app(slot->app_id);
        slot->win_id = find_window_for_app(slot->app_id);
        slot->name = s_pinned_apps[i].name;
        
        if (slot->win_id != 0) {
            BWE_Window* win = BWE_GetWindow(slot->win_id);
            slot->is_running = (win && win->state != BWE_STATE_DESTROYED);
            slot->is_focused = (slot->is_running && win->state != BWE_STATE_HIDDEN && BOS_GetFocus() == slot->win_id);
        } else {
            slot->is_running = false;
            slot->is_focused = false;
        }
        s_layout.app_count++;
    }

    // 2. Metrics & Dimensions (Matching Reference UI Image 1)
    int32_t capsule_h   = 52;
    int32_t corner_r    = 16;
    int32_t pad_left    = 10;
    int32_t pad_right   = 12;
    
    // Start Pill (Left Zone) — Icon-First ATOMS Emblem Button
    int32_t start_w     = 44;
    int32_t start_h     = 40;
    int32_t sep_w       = 14;
    
    // App Slots (Center Zone)
    int32_t app_cell_w  = 38;
    int32_t app_cell_h  = 38;
    int32_t app_spacing = 6;
    int32_t apps_total_w = (s_layout.app_count * (app_cell_w + app_spacing)) - app_spacing;
    
    // System Tray & RTC Clock (Right Zone)
    int32_t tray_icon_size = 22;
    int32_t tray_spacing   = 10;
    int32_t clock_text_w   = 84;
    int32_t tray_total_w   = tray_icon_size + tray_spacing + tray_icon_size + tray_spacing + clock_text_w;

    int32_t total_w = pad_left + start_w + sep_w + apps_total_w + sep_w + tray_total_w + pad_right;
    
    // Center horizontally, float 12px above bottom
    int32_t cx = (screen_w - total_w) / 2;
    if (cx < 10) cx = 10;
    int32_t cy = screen_h - capsule_h - 12;

    s_layout.capsule.x = cx;
    s_layout.capsule.y = cy;
    s_layout.capsule.width = total_w;
    s_layout.capsule.height = capsule_h;
    s_layout.corner_radius = corner_r;

    // Start Pill Geometry
    int32_t start_y = cy + (capsule_h - start_h) / 2;
    s_layout.start_pill.x = cx + pad_left;
    s_layout.start_pill.y = start_y;
    s_layout.start_pill.width = start_w;
    s_layout.start_pill.height = start_h;

    s_layout.start_icon.width = 28;
    s_layout.start_icon.height = 28;
    s_layout.start_icon.x = s_layout.start_pill.x + (start_w - 28) / 2;
    s_layout.start_icon.y = start_y + (start_h - 28) / 2;

    s_layout.start_text.x = 0;
    s_layout.start_text.y = 0;
    s_layout.start_text.width = 0;
    s_layout.start_text.height = 0;

    // App Slots Geometry
    int32_t app_cur_x = s_layout.start_pill.x + start_w + sep_w;
    int32_t app_cur_y = cy + (capsule_h - app_cell_h) / 2;

    for (uint32_t i = 0; i < s_layout.app_count; i++) {
        TaskbarAppSlot* slot = &s_layout.apps[i];
        slot->bounds.x = app_cur_x + i * (app_cell_w + app_spacing);
        slot->bounds.y = app_cur_y;
        slot->bounds.width = app_cell_w;
        slot->bounds.height = app_cell_h;
    }

    // System Tray Geometry
    int32_t tray_cur_x = app_cur_x + apps_total_w + sep_w;
    int32_t tray_icon_y = cy + (capsule_h - tray_icon_size) / 2;

    s_layout.tray_wifi.x = tray_cur_x;
    s_layout.tray_wifi.y = tray_icon_y;
    s_layout.tray_wifi.width = tray_icon_size;
    s_layout.tray_wifi.height = tray_icon_size;

    s_layout.tray_vol.x = tray_cur_x + tray_icon_size + tray_spacing;
    s_layout.tray_vol.y = tray_icon_y;
    s_layout.tray_vol.width = tray_icon_size;
    s_layout.tray_vol.height = tray_icon_size;

    s_layout.tray_clock.x = s_layout.tray_vol.x + tray_icon_size + tray_spacing;
    s_layout.tray_clock.y = cy;
    s_layout.tray_clock.width = clock_text_w;
    s_layout.tray_clock.height = capsule_h;
}

static inline void plot_pixel_clipped(const BVFramebuffer* fb, int32_t x, int32_t y, uint32_t color, const BWE_Rect* clip) {
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

// Accurate Euclidean rounded rectangle rendering with 1px smooth border
static void draw_rounded_pill(const BVFramebuffer* fb, int32_t cx, int32_t cy, int32_t cw, int32_t ch, int32_t r, uint32_t bg_color, uint32_t border_color, const BWE_Rect* clip) {
    int32_t left_cx = cx + r;
    int32_t right_cx = cx + cw - r - 1;
    int32_t top_cy = cy + r;
    int32_t bottom_cy = cy + ch - r - 1;
    int32_t r2_in = (r - 1) * (r - 1);
    int32_t r2_out = r * r;

    for (int32_t y = cy; y < cy + ch; y++) {
        for (int32_t x = cx; x < cx + cw; x++) {
            int32_t corner_cx = 0, corner_cy = 0;
            bool is_corner = false;

            if (x < left_cx && y < top_cy) {
                corner_cx = left_cx; corner_cy = top_cy; is_corner = true;
            } else if (x > right_cx && y < top_cy) {
                corner_cx = right_cx; corner_cy = top_cy; is_corner = true;
            } else if (x < left_cx && y > bottom_cy) {
                corner_cx = left_cx; corner_cy = bottom_cy; is_corner = true;
            } else if (x > right_cx && y > bottom_cy) {
                corner_cx = right_cx; corner_cy = bottom_cy; is_corner = true;
            }

            if (is_corner) {
                int32_t dx = x - corner_cx;
                int32_t dy = y - corner_cy;
                int32_t dist_sq = dx * dx + dy * dy;
                if (dist_sq > r2_out) {
                    continue; // Outside arc
                }
                if (dist_sq >= r2_in && border_color != 0) {
                    plot_pixel_clipped(fb, x, y, border_color, clip);
                } else if (bg_color != 0) {
                    plot_pixel_clipped(fb, x, y, bg_color, clip);
                }
            } else {
                bool is_edge = (x == cx || x == cx + cw - 1 || y == cy || y == cy + ch - 1);
                if (is_edge && border_color != 0) {
                    plot_pixel_clipped(fb, x, y, border_color, clip);
                } else if (bg_color != 0) {
                    plot_pixel_clipped(fb, x, y, bg_color, clip);
                }
            }
        }
    }
}

static const char* s_month_names[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void task_panel_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    BWE_Rect clip;
    if (!BWE_GetClip(&clip)) {
        clip.x = 0;
        clip.y = 0;
        clip.width = (int32_t)fb->width;
        clip.height = (int32_t)fb->height;
    }

    // 1. Single Authoritative Geometry Computation
    task_panel_compute_layout((int32_t)fb->width, (int32_t)fb->height);

    self->screen_bounds.x = s_layout.capsule.x;
    self->screen_bounds.y = s_layout.capsule.y;
    self->screen_bounds.width = s_layout.capsule.width;
    self->screen_bounds.height = s_layout.capsule.height;

    // Palette tokens
    uint32_t capsule_bg   = 0xCC111827; // Translucent dark slate-900 (~80% opacity)
    uint32_t capsule_rim  = 0x33FFFFFF; // 1px subtle glass highlight rim
    uint32_t start_pill_bg= (s_hover_index == 0 || g_start_menu_open) ? 0x4038BDF8 : 0x26334155;
    uint32_t start_pill_bd= (s_hover_index == 0 || g_start_menu_open) ? 0x8038BDF8 : 0x3394A3B8;
    uint32_t accent_cyan  = 0xFF38BDF8;

    // 2. Render Main Floating Rounded Capsule
    draw_rounded_pill(fb, s_layout.capsule.x, s_layout.capsule.y,
                      s_layout.capsule.width, s_layout.capsule.height,
                      s_layout.corner_radius, capsule_bg, capsule_rim, &clip);

    // 3. Render Left Zone: Start Pill Button using ATOMS Icon Engine
    IconState start_state = ICON_STATE_NORMAL;
    if (g_start_menu_open) {
        start_state = ICON_STATE_ACTIVE;
    } else if (s_hover_index == 0) {
        start_state = ICON_STATE_HOVER;
    }

    uint32_t start_bg = 0x331E293B; // Subtle dark slate glass
    uint32_t start_bd = 0x3364748B; // 1px subtle glass highlight border
    if (start_state == ICON_STATE_ACTIVE) {
        start_bg = 0x6638BDF8; // Active radiant cyan
        start_bd = 0xFF38BDF8;
    } else if (start_state == ICON_STATE_HOVER) {
        start_bg = 0x4D38BDF8; // Brightened hover glass
        start_bd = 0x9938BDF8;
    }

    draw_rounded_pill(fb, s_layout.start_pill.x, s_layout.start_pill.y,
                      s_layout.start_pill.width, s_layout.start_pill.height,
                      12, start_bg, start_bd, &clip);

    IconRenderContext icon_ctx;
    icon_ctx.x = s_layout.start_icon.x;
    icon_ctx.y = s_layout.start_icon.y;
    icon_ctx.width = s_layout.start_icon.width;
    icon_ctx.height = s_layout.start_icon.height;
    icon_ctx.state = start_state;
    icon_ctx.accent_color = 0;
    icon_ctx.clip = &clip;

    IconEngine_Render(fb, ICON_ID_ATOMS_START, &icon_ctx);

    // 4. Render Center Zone: Application Icons & Running Indicators
    for (uint32_t i = 0; i < s_layout.app_count; i++) {
        TaskbarAppSlot* slot = &s_layout.apps[i];
        bool is_hovered = (s_hover_index == (int32_t)(i + 1));

        // Hover / Focused Cell Background
        if (is_hovered) {
            draw_rounded_pill(fb, slot->bounds.x, slot->bounds.y, slot->bounds.width, slot->bounds.height,
                              8, slot->is_focused ? 0x4D38BDF8 : 0x26FFFFFF, 0x33FFFFFF, &clip);
        } else if (slot->is_focused) {
            draw_rounded_pill(fb, slot->bounds.x, slot->bounds.y, slot->bounds.width, slot->bounds.height,
                              8, 0x2638BDF8, 0, &clip);
        }

        // Draw Transparent HD App Icon (32x32 centered inside 38x38 slot)
        int32_t icon_x = slot->bounds.x + (slot->bounds.width - 32) / 2;
        int32_t icon_y = slot->bounds.y + (slot->bounds.height - 32) / 2 - 2;

        IconState app_state = ICON_STATE_NORMAL;
        if (slot->is_focused) {
            app_state = ICON_STATE_ACTIVE;
        } else if (is_hovered) {
            app_state = ICON_STATE_HOVER;
        }

        IconRenderContext app_ctx;
        app_ctx.x = icon_x;
        app_ctx.y = icon_y;
        app_ctx.width = 32;
        app_ctx.height = 32;
        app_ctx.state = app_state;
        app_ctx.accent_color = 0;
        app_ctx.clip = &clip;

        IconId app_icon_id = ICON_ID_NONE;
        switch (slot->app_id) {
            case APP_ID_EXPLORER:    app_icon_id = ICON_ID_EXPLORER; break;
            case APP_ID_TERMINAL:    app_icon_id = ICON_ID_TERMINAL; break;
            case APP_ID_NOTES:       app_icon_id = ICON_ID_NOTES; break;
            case APP_ID_CALCULATOR:  app_icon_id = ICON_ID_CALCULATOR; break;
            case APP_ID_SETTINGS:    app_icon_id = ICON_ID_SETTINGS; break;
            case APP_ID_MUSIC:       app_icon_id = ICON_ID_MEDIA_PLAYER; break;
            case APP_ID_TMH:         app_icon_id = ICON_ID_TASK_MANAGER; break;
            case APP_ID_ATRIX:       app_icon_id = ICON_ID_ATRIX; break;
            case APP_ID_GRAPH_3D:    app_icon_id = ICON_ID_GRAPH_3D; break;
            case APP_ID_DOOM:        app_icon_id = ICON_ID_DOOM; break;
            default:                 app_icon_id = ICON_ID_FOLDER; break;
        }

        if (!IconEngine_Render(fb, app_icon_id, &app_ctx)) {
            BOAsset_DrawAsset(slot->asset_id, icon_x, icon_y, 32, 32);
        }

        // Running & Focused Active Indicators (Pill beneath icon)
        if (slot->is_focused) {
            // Active application: 18px horizontal indicator pill
            int32_t iw = 18;
            int32_t ih = 3;
            int32_t ix = slot->bounds.x + (slot->bounds.width - iw) / 2;
            int32_t iy = slot->bounds.y + slot->bounds.height - 2;
            draw_rounded_pill(fb, ix, iy, iw, ih, 2, accent_cyan, 0, &clip);
        } else if (slot->is_running) {
            // Background running application: 5px subtle dot
            int32_t iw = 5;
            int32_t ih = 3;
            int32_t ix = slot->bounds.x + (slot->bounds.width - iw) / 2;
            int32_t iy = slot->bounds.y + slot->bounds.height - 2;
            draw_rounded_pill(fb, ix, iy, iw, ih, 2, 0x9994A3B8, 0, &clip);
        }
    }

    // 5. Render Right Zone: System Tray (LAN / Ethernet, Volume)
    IconRenderContext tray_ctx;
    tray_ctx.width = s_layout.tray_wifi.width;
    tray_ctx.height = s_layout.tray_wifi.height;
    tray_ctx.state = ICON_STATE_NORMAL;
    tray_ctx.accent_color = 0;
    tray_ctx.clip = &clip;

    tray_ctx.x = s_layout.tray_wifi.x;
    tray_ctx.y = s_layout.tray_wifi.y;
    if (!IconEngine_Render(fb, ICON_ID_SYS_LAN, &tray_ctx)) {
        BOAsset_DrawAsset(ICON_SYS_WIFI_CONN, s_layout.tray_wifi.x, s_layout.tray_wifi.y, tray_ctx.width, tray_ctx.height);
    }

    tray_ctx.x = s_layout.tray_vol.x;
    tray_ctx.y = s_layout.tray_vol.y;
    if (!IconEngine_Render(fb, ICON_ID_SYS_VOLUME, &tray_ctx)) {
        BOAsset_DrawAsset(ICON_SYS_VOL_NORM, s_layout.tray_vol.x, s_layout.tray_vol.y, tray_ctx.width, tray_ctx.height);
    }

    // 6. Render Right Zone: RTC Clock & Date
    RTCDateTime dt;
    if (!rtc_read_datetime(&dt)) {
        dt.hour = 12; dt.minute = 40; dt.second = 0;
        dt.month = 8; dt.day = 22; dt.year = 2026;
    }

    int display_hour = dt.hour % 12;
    if (display_hour == 0) display_hour = 12;
    const char* ampm = (dt.hour >= 12) ? "PM" : "AM";

    char time_buf[16];
    time_buf[0] = (display_hour / 10) ? ('0' + (display_hour / 10)) : ' ';
    time_buf[1] = '0' + (display_hour % 10);
    time_buf[2] = ':';
    time_buf[3] = '0' + (dt.minute / 10);
    time_buf[4] = '0' + (dt.minute % 10);
    time_buf[5] = ' ';
    time_buf[6] = ampm[0];
    time_buf[7] = ampm[1];
    time_buf[8] = '\0';

    char date_buf[16];
    date_buf[0] = (dt.day / 10) ? ('0' + (dt.day / 10)) : ' ';
    date_buf[1] = '0' + (dt.day % 10);
    date_buf[2] = ' ';
    const char* mon_str = (dt.month >= 1 && dt.month <= 12) ? s_month_names[dt.month - 1] : "Aug";
    date_buf[3] = mon_str[0];
    date_buf[4] = mon_str[1];
    date_buf[5] = mon_str[2];
    date_buf[6] = '\0';

    if (s_hover_index == 1000) {
        draw_rounded_pill(fb, s_layout.tray_clock.x - 4, s_layout.capsule.y + 6,
                          s_layout.tray_clock.width + 6, s_layout.capsule.height - 12,
                          8, 0x26FFFFFF, 0, &clip);
    }

    BWE_DrawText(fb, time_buf, s_layout.tray_clock.x + 2, s_layout.capsule.y + 9, 0xFFFFFFFF, 0);
    BWE_DrawText(fb, date_buf, s_layout.tray_clock.x + 10, s_layout.capsule.y + 26, 0xFF94A3B8, 0);
}

static void task_panel_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    if (event->type == BWE_EVENT_MOUSE_MOVE) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        int32_t new_hover = -1;
        // Check Start Pill Hover
        if (mx >= s_layout.start_pill.x && mx < s_layout.start_pill.x + s_layout.start_pill.width &&
            my >= s_layout.start_pill.y && my < s_layout.start_pill.y + s_layout.start_pill.height) {
            new_hover = 0;
        }
        // Check Clock/Tray Hover
        else if (mx >= s_layout.tray_clock.x && mx < s_layout.tray_clock.x + s_layout.tray_clock.width &&
                 my >= s_layout.capsule.y && my < s_layout.capsule.y + s_layout.capsule.height) {
            new_hover = 1000;
        }
        // Check App Icons Hover
        else {
            for (uint32_t i = 0; i < s_layout.app_count; i++) {
                TaskbarAppSlot* slot = &s_layout.apps[i];
                if (mx >= slot->bounds.x && mx < slot->bounds.x + slot->bounds.width &&
                    my >= slot->bounds.y && my < slot->bounds.y + slot->bounds.height) {
                    new_hover = (int32_t)(i + 1);
                    break;
                }
            }
        }

        if (new_hover != s_hover_index) {
            s_hover_index = new_hover;
            BWE_InvalidateWindow(window_id);
        }
        return;
    }

    if (event->type == BWE_EVENT_MOUSE_LEAVE) {
        if (s_hover_index != -1) {
            s_hover_index = -1;
            BWE_InvalidateWindow(window_id);
        }
        return;
    }

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        // Check Start Menu Click
        if (mx >= s_layout.start_pill.x && mx < s_layout.start_pill.x + s_layout.start_pill.width &&
            my >= s_layout.start_pill.y && my < s_layout.start_pill.y + s_layout.start_pill.height) {
            g_start_menu_open = !g_start_menu_open;
            if (g_start_menu_open) {
                extern void StartMenu_RefreshCache(void);
                StartMenu_RefreshCache();
                BOS_Show(g_start_menu_win_id);
                BOS_SetFocus(g_start_menu_win_id);
            } else {
                BOS_Hide(g_start_menu_win_id);
            }

            BWE_InvalidateWindow(window_id);
            return;
        }

        // Check App Icon Click
        for (uint32_t i = 0; i < s_layout.app_count; i++) {
            TaskbarAppSlot* slot = &s_layout.apps[i];
            if (mx >= slot->bounds.x && mx < slot->bounds.x + slot->bounds.width &&
                my >= slot->bounds.y && my < slot->bounds.y + slot->bounds.height) {
                
                uint32_t win_id = slot->win_id;
                uint32_t app_id = slot->app_id;

                if (g_start_menu_open) {
                    g_start_menu_open = false;
                    BOS_Hide(g_start_menu_win_id);
                }

                if (win_id != 0) {
                    BWE_Window* win = BWE_GetWindow(win_id);
                    if (win && win->state != BWE_STATE_DESTROYED) {
                        if (win->state == BWE_STATE_HIDDEN) {
                            BOS_Show(win_id);
                            BOS_SetFocus(win_id);
                            BWE_BringToFront(win_id);
                        } else if (BOS_GetFocus() == win_id) {
                            BOS_Hide(win_id);
                        } else {
                            BOS_Show(win_id);
                            BOS_SetFocus(win_id);
                            BWE_BringToFront(win_id);
                        }
                    } else {
                        extern void horse_launch(uint32_t id);
                        horse_launch(app_id);
                    }
                } else {
                    extern void horse_launch(uint32_t id);
                    horse_launch(app_id);
                }
                BWE_InvalidateWindow(window_id);
                return;
            }
        }
    }
}

void TaskPanel_Initialize(void) {
    IconEngine_Initialize();

    uint32_t scr_w = g_kernel_screen_width > 0 ? g_kernel_screen_width : 1024;
    uint32_t scr_h = g_kernel_screen_height > 0 ? g_kernel_screen_height : 768;
    
    task_panel_compute_layout((int32_t)scr_w, (int32_t)scr_h);

    BOS_CreatePanel(BWE_DESKTOP_ID, s_layout.capsule.x, s_layout.capsule.y,
                    s_layout.capsule.width, s_layout.capsule.height, 0x00000000, &g_task_panel_win_id);
    BWE_Window* tb = BWE_GetWindow(g_task_panel_win_id);
    if (tb) {
        tb->type = BWE_TYPE_TASKBAR;
        tb->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST | BWE_WINDOW_TRANSPARENT;
        tb->on_render = task_panel_render_callback;
        tb->on_event = task_panel_event_callback;
        tb->is_dirty = true;
        BOS_Show(g_task_panel_win_id);
        BWE_BringToFront(g_task_panel_win_id);
        BWE_UpdateZOrders();
    }
}

void TaskPanel_Update(void) {
    if (g_task_panel_win_id != 0) {
        BWE_InvalidateWindow(g_task_panel_win_id);
    }
}
