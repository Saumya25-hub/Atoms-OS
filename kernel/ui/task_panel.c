#include "task_panel.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/display/agdae/agdae.h"
#include "kernel/ui/boasset/boasset.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/engine/horse_engine.h"

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

// Asset resolution helper with generic fallback
static uint32_t get_asset_for_app(uint32_t app_id) {
    if (app_id >= 999000) {
        uint32_t win_id = app_id - 999000;
        BWE_Window* win = BWE_GetWindow(win_id);
        if (win) {
            const char* title = (win->title[0] != '\0') ? win->title : win->control_data.button.text;
            if (contains_str(title, "Explorer")) return ICON_EXPLORER;
            if (contains_str(title, "Terminal")) return ICON_TERMINAL;
            if (contains_str(title, "Settings") || contains_str(title, "Personalization")) return ICON_SETTINGS;
            if (contains_str(title, "Calc")) return ICON_CALCULATOR;
            if (contains_str(title, "Music")) return ICON_MUSIC;
        }
        return ICON_FILE;
    }
    switch (app_id) {
        case APP_ID_EXPLORER:   return ICON_EXPLORER;
        case APP_ID_TERMINAL:   return ICON_TERMINAL;
        case APP_ID_SETTINGS:   return ICON_SETTINGS;
        case APP_ID_CALCULATOR: return ICON_CALCULATOR;
        case APP_ID_MUSIC:      return ICON_MUSIC;
        case APP_ID_ATRIX:      return ICON_ATRIX;
        case APP_ID_DOOM:       return ICON_DOOM;
        case APP_ID_STRESS_TEST:return ICON_STRESS_TEST;
        case APP_ID_INPUT_LAB:  return ICON_INPUT_LAB;
        case APP_ID_GRAPH_3D:   return ICON_GRAPH_3D;
        case APP_ID_IMAGE_VIEWER:return ICON_FILE;
        case APP_ID_SANDBOX:    return ICON_FILE;
        default:                return ICON_FILE;
    }
}

typedef struct {
    uint32_t app_id;
    uint32_t asset_id;
    const char* name;
} TaskAppItem;

static const TaskAppItem s_pinned_apps[] = {
    { APP_ID_EXPLORER,   ICON_EXPLORER,   "Explorer" },
    { APP_ID_TERMINAL,   ICON_TERMINAL,   "Terminal" },
    { APP_ID_SETTINGS,   ICON_SETTINGS,   "Settings" },
    { APP_ID_CALCULATOR, ICON_CALCULATOR, "Calculator" },
    { APP_ID_MUSIC,      ICON_MUSIC,      "Music" },
    { APP_ID_ATRIX,      ICON_ATRIX,      "ATRIX Browser" }
};
#define NUM_PINNED_APPS (sizeof(s_pinned_apps)/sizeof(s_pinned_apps[0]))

#define MAX_CAPSULE_APPS 32

static uint32_t s_app_ids[MAX_CAPSULE_APPS];
static uint32_t s_asset_ids[MAX_CAPSULE_APPS];
static uint32_t s_app_win_ids[MAX_CAPSULE_APPS];
static uint32_t s_app_count = 0;

static int32_t s_capsule_x = 0;
static int32_t s_capsule_y = 0;
static int32_t s_capsule_w = 0;
static int32_t s_capsule_h = 54; // Slightly bigger comfortable height
static int32_t s_start_x = 0;
static int32_t s_sep_x = 0;
static int32_t s_app_start_x = 0;
static int32_t s_cell_y = 0;
static int32_t s_hover_index = -1;

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

static void draw_capsule_shape(const BVFramebuffer* fb, int32_t cx, int32_t cy, int32_t cw, int32_t ch, uint32_t bg_color, uint32_t border_color, const BWE_Rect* clip) {
    int32_t r = ch / 2;
    if (r > 24) r = 24;

    for (int32_t y = cy; y < cy + ch; y++) {
        for (int32_t x = cx; x < cx + cw; x++) {
            if (is_outside_rounded_rect(x, y, cx, cy, cw, ch, r)) continue;

            bool is_edge = (x == cx || x == cx + cw - 1 || y == cy || y == cy + ch - 1 ||
                            is_outside_rounded_rect(x - 1, y, cx, cy, cw, ch, r) ||
                            is_outside_rounded_rect(x + 1, y, cx, cy, cw, ch, r) ||
                            is_outside_rounded_rect(x, y - 1, cx, cy, cw, ch, r) ||
                            is_outside_rounded_rect(x, y + 1, cx, cy, cw, ch, r));

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

static uint32_t find_window_for_app(uint32_t app_id) {
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED && win->id != 0 && win->parent_id == BWE_DESKTOP_ID && win->type == BWE_TYPE_WINDOW) {
            if (win->id == g_task_panel_win_id || win->id == g_start_menu_win_id) continue;
            uint32_t win_app_id = (uint32_t)(uintptr_t)win->user_data;
            if (win_app_id == app_id) return win->id;
            
            const char* title = win->control_data.button.text;
            if (app_id == APP_ID_EXPLORER && contains_str(title, "Explorer")) return win->id;
            if (app_id == APP_ID_TERMINAL && contains_str(title, "Terminal")) return win->id;
            if (app_id == APP_ID_SETTINGS && contains_str(title, "Settings")) return win->id;
            if (app_id == APP_ID_CALCULATOR && contains_str(title, "Calc")) return win->id;
            if (app_id == APP_ID_MUSIC && contains_str(title, "Music")) return win->id;
            if (app_id == APP_ID_ATRIX && contains_str(title, "ATRIX")) return win->id;
            if (app_id == APP_ID_DOOM && contains_str(title, "DOOM")) return win->id;
            if (app_id == APP_ID_STRESS_TEST && contains_str(title, "Stress")) return win->id;
            if (app_id == APP_ID_INPUT_LAB && contains_str(title, "Input")) return win->id;
            if (app_id == APP_ID_GRAPH_3D && (contains_str(title, "Graph") || contains_str(title, "3D"))) return win->id;
        }
    }
    return 0;
}

static uint32_t infer_app_id_for_window(BWE_Window* win) {
    if (!win) return 0;
    uint32_t win_app_id = (uint32_t)(uintptr_t)win->user_data;
    if (win_app_id > 0 && win_app_id <= 100) return win_app_id;

    const char* title = (win->title[0] != '\0') ? win->title : win->control_data.button.text;
    if (contains_str(title, "Explorer")) return APP_ID_EXPLORER;
    if (contains_str(title, "Terminal")) return APP_ID_TERMINAL;
    if (contains_str(title, "Settings") || contains_str(title, "Personalization")) return APP_ID_SETTINGS;
    if (contains_str(title, "Calc")) return APP_ID_CALCULATOR;
    if (contains_str(title, "Music")) return APP_ID_MUSIC;
    if (contains_str(title, "ATRIX")) return APP_ID_ATRIX;
    if (contains_str(title, "DOOM")) return APP_ID_DOOM;
    if (contains_str(title, "Stress")) return APP_ID_STRESS_TEST;
    if (contains_str(title, "Input")) return APP_ID_INPUT_LAB;
    if (contains_str(title, "Graph") || contains_str(title, "3D")) return APP_ID_GRAPH_3D;

    return 999000 + win->id;
}

static void task_panel_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    BWE_Rect clip = {0, 0, (int32_t)fb->width, (int32_t)fb->height};

    // 1. Pinned Apps
    s_app_count = 0;
    for (size_t i = 0; i < NUM_PINNED_APPS && s_app_count < MAX_CAPSULE_APPS; i++) {
        s_app_ids[s_app_count] = s_pinned_apps[i].app_id;
        s_asset_ids[s_app_count] = get_asset_for_app(s_pinned_apps[i].app_id);
        s_app_win_ids[s_app_count] = find_window_for_app(s_pinned_apps[i].app_id);
        s_app_count++;
    }

    // 2. Dynamic Running Apps Discovery
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS && s_app_count < MAX_CAPSULE_APPS; i++) {
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED && win->id != 0 && win->parent_id == BWE_DESKTOP_ID && win->type == BWE_TYPE_WINDOW) {
            if (win->id == g_task_panel_win_id || win->id == g_start_menu_win_id) continue;
            
            uint32_t app_id = infer_app_id_for_window(win);
            if (app_id == 0) continue;

            bool exists = false;
            for (uint32_t j = 0; j < s_app_count; j++) {
                if (s_app_ids[j] == app_id) {
                    exists = true;
                    if (s_app_win_ids[j] == 0) s_app_win_ids[j] = win->id;
                    break;
                }
            }

            if (!exists) {
                s_app_ids[s_app_count] = app_id;
                s_asset_ids[s_app_count] = get_asset_for_app(app_id);
                s_app_win_ids[s_app_count] = win->id;
                s_app_count++;
            }
        }
    }

    // Geometry Calculation (Slightly Bigger Taskbar Capsule)
    int32_t cell_w = 44;
    int32_t cell_h = 44;
    int32_t spacing = 10;
    int32_t pad_left = 16;
    int32_t pad_right = 16;
    int32_t start_w = 44;
    int32_t sep_w = 14;

    int32_t total_w = pad_left + start_w + sep_w + (s_app_count * (cell_w + spacing)) + pad_right - spacing;
    int32_t total_h = 54;

    s_capsule_w = total_w;
    s_capsule_h = total_h;
    s_capsule_x = (self->screen_bounds.width - total_w) / 2;
    s_capsule_y = self->screen_bounds.height - total_h - 12;

    int32_t abs_cx = self->screen_bounds.x + s_capsule_x;
    int32_t abs_cy = self->screen_bounds.y + s_capsule_y;

    uint32_t tb_bg     = BOTHEME_GetColor(BOTHEME_TASKBAR_BG);
    uint32_t tb_border = BOTHEME_GetColor(BOTHEME_TASKBAR_BORDER);
    uint32_t accent_col= BOTHEME_GetColor(BOTHEME_ACCENT_PRIMARY);

    uint32_t capsule_bg = 0xE60F172A;
    if ((tb_bg & 0x00FFFFFF) != 0) {
        capsule_bg = 0xEE000000 | (tb_bg & 0x00FFFFFF);
    }

    // 1. Render Main Floating Capsule
    draw_capsule_shape(fb, abs_cx, abs_cy, total_w, total_h, capsule_bg, tb_border, &clip);

    // 2. Render ATOMS Start Logo Button (Far Left - Transparent PNG Emblem)
    s_start_x = abs_cx + pad_left;
    s_cell_y  = abs_cy + (total_h - cell_h) / 2;

    if (s_hover_index == 0 || g_start_menu_open) {
        draw_rounded_box(fb, s_start_x, s_cell_y, cell_w, cell_h, 8, g_start_menu_open ? 0x4D3B82F6 : 0x2AFFFFFF, &clip);
    }

    if (!BOAsset_DrawAsset(ASSET_LOGO, s_start_x + 4, s_cell_y + 4, 36, 36)) {
        BWE_FillRect(fb, s_start_x + 8, s_cell_y + 8, 28, 28, 0xFF2563EB);
        BWE_DrawText(fb, "A", s_start_x + 16, s_cell_y + 14, 0xFFFFFFFF, 0);
    }

    // 3. Render 1px Vertical Separator Line |
    s_sep_x = s_start_x + start_w + (sep_w / 2);
    for (int32_t sy = abs_cy + 10; sy < abs_cy + total_h - 10; sy++) {
        plot_pixel(fb, s_sep_x, sy, 0x44FFFFFF, &clip);
    }

    // 4. Render App Icons
    s_app_start_x = s_sep_x + (sep_w / 2) + 3;

    for (uint32_t i = 0; i < s_app_count; i++) {
        int32_t cell_x = s_app_start_x + i * (cell_w + spacing);
        uint32_t win_id = s_app_win_ids[i];
        BWE_Window* win = win_id ? BWE_GetWindow(win_id) : 0;
        
        bool is_running = (win != 0 && win->state != BWE_STATE_DESTROYED);
        bool is_focused = (is_running && win->state != BWE_STATE_HIDDEN && BOS_GetFocus() == win_id);
        bool is_hovered = (s_hover_index == (int32_t)(i + 1));

        if (is_hovered) {
            uint32_t hover_col = is_focused ? 0x553B82F6 : 0x33FFFFFF;
            draw_rounded_box(fb, cell_x, s_cell_y, cell_w, cell_h, 8, hover_col, &clip);
        } else if (is_focused) {
            draw_rounded_box(fb, cell_x, s_cell_y, cell_w, cell_h, 8, 0x383B82F6, &clip);
        }

        if (!BOAsset_DrawAsset(s_asset_ids[i], cell_x + 4, s_cell_y + 4, 36, 36)) {
            BWE_FillRect(fb, cell_x + 8, s_cell_y + 8, 28, 28, 0xFF3B82F6);
        }

        if (is_focused) {
            int32_t iw = 24;
            int32_t ih = 3;
            int32_t ix = cell_x + (cell_w - iw) / 2;
            int32_t iy = s_cell_y + cell_h - 2;
            draw_rounded_box(fb, ix, iy, iw, ih, 2, accent_col, &clip);
        } else if (is_running) {
            int32_t iw = 6;
            int32_t ih = 3;
            int32_t ix = cell_x + (cell_w - iw) / 2;
            int32_t iy = s_cell_y + cell_h - 2;
            draw_rounded_box(fb, ix, iy, iw, ih, 2, accent_col, &clip);
        }
    }
}

static void task_panel_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    int32_t cell_w = 44;
    int32_t spacing = 10;

    if (event->type == BWE_EVENT_MOUSE_MOVE) {
        int32_t mx = event->data.mouse.x - self->screen_bounds.x;
        int32_t my = event->data.mouse.y - self->screen_bounds.y;

        int32_t new_hover = -1;
        if (mx >= s_capsule_x && mx < s_capsule_x + s_capsule_w && my >= s_capsule_y && my < s_capsule_y + s_capsule_h) {
            if (mx >= s_start_x - self->screen_bounds.x && mx < s_start_x - self->screen_bounds.x + cell_w &&
                my >= s_cell_y - self->screen_bounds.y && my < s_cell_y - self->screen_bounds.y + cell_w) {
                new_hover = 0;
            } else {
                for (uint32_t i = 0; i < s_app_count; i++) {
                    int32_t cell_x = s_app_start_x - self->screen_bounds.x + i * (cell_w + spacing);
                    if (mx >= cell_x && mx < cell_x + cell_w &&
                        my >= s_cell_y - self->screen_bounds.y && my < s_cell_y - self->screen_bounds.y + cell_w) {
                        new_hover = (int32_t)(i + 1);
                        break;
                    }
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
        int32_t mx = event->data.mouse.x - self->screen_bounds.x;
        int32_t my = event->data.mouse.y - self->screen_bounds.y;

        // Check Start Menu Click
        if (mx >= s_start_x - self->screen_bounds.x && mx < s_start_x - self->screen_bounds.x + cell_w &&
            my >= s_cell_y - self->screen_bounds.y && my < s_cell_y - self->screen_bounds.y + cell_w) {
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
        for (uint32_t i = 0; i < s_app_count; i++) {
            int32_t cell_x = s_app_start_x - self->screen_bounds.x + i * (cell_w + spacing);
            if (mx >= cell_x && mx < cell_x + cell_w &&
                my >= s_cell_y - self->screen_bounds.y && my < s_cell_y - self->screen_bounds.y + cell_w) {
                
                uint32_t win_id = s_app_win_ids[i];
                uint32_t app_id = s_app_ids[i];

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
                        } else if (BOS_GetFocus() == win_id) {
                            BOS_Hide(win_id);
                        } else {
                            BOS_SetFocus(win_id);
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
    const AGDAE_Metrics* metrics = AGDAE_GetMetrics();
    
    int32_t px = 0;
    int32_t py = metrics->desktop_rect.height - 66;
    int32_t panel_width = metrics->desktop_rect.width;
    int32_t panel_height = 66;

    BOS_CreatePanel(BWE_DESKTOP_ID, px, py, panel_width, panel_height, 0x00000000, &g_task_panel_win_id);
    BWE_Window* tb = BWE_GetWindow(g_task_panel_win_id);
    if (tb) {
        tb->type = BWE_TYPE_TASKBAR;
        tb->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST | BWE_WINDOW_TRANSPARENT;
        tb->on_render = task_panel_render_callback;
        tb->on_event = task_panel_event_callback;
    }
}

void TaskPanel_Update(void) {
    if (g_task_panel_win_id != 0) {
        BWE_InvalidateWindow(g_task_panel_win_id);
    }
}
