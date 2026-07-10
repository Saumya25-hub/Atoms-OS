#include "task_panel.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/display/agdae/agdae.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern BWE_Window g_windows[];

uint32_t g_task_panel_win_id = 0;
extern uint32_t g_start_menu_win_id;
extern bool g_start_menu_open;

// Port IO declarations for RTC
extern void io_out8(uint16_t port, uint8_t data);
extern uint8_t io_in8(uint16_t port);

static uint8_t rtc_read(uint8_t reg) {
    io_out8(0x70, reg);
    return io_in8(0x71);
}

static void read_rtc_time(int* hour, int* minute) {
    int timeout = 1000;
    while (timeout-- > 0) {
        io_out8(0x70, 0x0A);
        if (!(io_in8(0x71) & 0x80)) break;
    }
    
    uint8_t min = rtc_read(0x02);
    uint8_t hr  = rtc_read(0x04);
    uint8_t regB = rtc_read(0x0B);
    
    if (!(regB & 0x04)) {
        min = ((min & 0xF0) >> 4) * 10 + (min & 0x0F);
        hr  = ((hr & 0xF0) >> 4) * 10 + (hr & 0x0F);
    }
    
    if (!(regB & 0x02) && (hr & 0x80)) {
        hr = ((hr & 0x7F) + 12) % 24;
    }
    
    *hour = hr;
    *minute = min;
}

static void format_two_digits(int val, char* out) {
    if (val < 0) val = 0;
    if (val > 99) val = 99;
    out[0] = (val / 10) + '0';
    out[1] = (val % 10) + '0';
    out[2] = '\0';
}

// Telemetry Variables
uint32_t g_tp_total_buttons = 0;
uint32_t g_tp_visible_buttons = 0;
uint32_t g_tp_overflow_count = 0;
uint32_t g_tp_panel_width = 0;
uint32_t g_tp_free_space = 0;
uint32_t g_tp_layout_passes = 0;

static void task_panel_truncate_text(const char* src, char* dest, int max_chars) {
    if (max_chars <= 0) {
        dest[0] = '\0';
        return;
    }
    int len = strlen(src);
    if (len <= max_chars) {
        strncpy(dest, src, max_chars + 1);
        return;
    }
    if (max_chars <= 3) {
        for (int i = 0; i < max_chars; i++) dest[i] = '.';
        dest[max_chars] = '\0';
        return;
    }
    strncpy(dest, src, max_chars - 3);
    dest[max_chars - 3] = '.';
    dest[max_chars - 2] = '.';
    dest[max_chars - 1] = '.';
    dest[max_chars] = '\0';
}

#define MAX_TASK_BUTTONS 128
typedef struct {
    uint32_t win_id;
    int32_t x;
    int32_t w;
} TaskBtn;
static TaskBtn s_task_buttons[MAX_TASK_BUTTONS];
static uint32_t s_task_btn_count = 0;

static void task_panel_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Fill Panel Background (Dark, solid)
    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, 0xFF0F172A);

    // Render Start Button
    BWE_FillRect(fb, self->screen_bounds.x + 10, self->screen_bounds.y + 10, 80, 30, g_start_menu_open ? 0xFF2563EB : 0xFF1E293B);
    BWE_DrawText(fb, "ATOMS", self->screen_bounds.x + 30, self->screen_bounds.y + 18, 0xFFFFFFFF, 0);

    // Rebuild the task buttons list based on current active windows
    s_task_btn_count = 0;
    
    g_tp_panel_width = self->screen_bounds.width;
    g_tp_layout_passes++;
    
    // Pass 1: Count active windows
    uint32_t active_win_ids[BWE_MAX_WINDOWS];
    uint32_t active_count = 0;
    
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED && win->id != 0 && win->parent_id == BWE_DESKTOP_ID && win->type == BWE_TYPE_WINDOW) {
            if (win->id == g_task_panel_win_id || win->id == g_start_menu_win_id) continue;
            active_win_ids[active_count++] = win->id;
        }
    }
    
    static uint32_t s_last_active_count = 0xFFFFFFFF;
    if (active_count != s_last_active_count && s_last_active_count != 0xFFFFFFFF) {
        extern void display_print(const char*);
        extern void display_print_dec(uint32_t);
        display_print("[AUDIT] --- TASK PANEL REGISTRATION PIPELINE AUDIT ---\n");
        for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
            BWE_Window* w = &g_windows[i];
            if (w->state != BWE_STATE_DESTROYED && w->id != 0) {
                display_print("[AUDIT] Live Surface ID: ");
                display_print_dec(w->id);
                display_print("\n[AUDIT]   Parent ID: ");
                display_print_dec(w->parent_id);
                display_print("\n[AUDIT]   Type: ");
                display_print_dec((uint32_t)w->type);
                if (w->id == g_task_panel_win_id || w->id == g_start_menu_win_id) {
                    display_print("  (Is Shell Control)");
                }
                display_print("\n");
            }
        }
        display_print("[AUDIT] --- END AUDIT ---\n");
    }
    s_last_active_count = active_count;
    
    g_tp_total_buttons = active_count;
    
    // Pass 2: Calculate dynamic width
    int32_t available_width = self->screen_bounds.width - 100 - 70; // 100 start btn, 70 clock
    if (available_width < 0) available_width = 0;
    
    int32_t btn_w = 150; // default preferred
    bool overflow = false;
    
    if (active_count > 0) {
        int32_t required_space = active_count * (btn_w + 10) - 10;
        if (required_space > available_width) {
            btn_w = (available_width + 10) / active_count - 10;
        }
    }
    
    // Cap minimum to 40px
    uint32_t render_count = active_count;
    if (btn_w < 40) {
        btn_w = 40;
        render_count = (available_width + 10) / 50; // 40 width + 10 spacing
        overflow = true;
    }
    
    if (render_count > MAX_TASK_BUTTONS) render_count = MAX_TASK_BUTTONS;
    
    g_tp_visible_buttons = render_count;
    g_tp_overflow_count = active_count > render_count ? active_count - render_count : 0;
    
    int32_t offset_x = 100;
    int32_t used_width = 0;
    
    for (uint32_t i = 0; i < render_count; i++) {
        BWE_Window* win = BWE_GetWindow(active_win_ids[i]);
        if (!win) continue;
        
        s_task_buttons[s_task_btn_count].win_id = win->id;
        s_task_buttons[s_task_btn_count].x = offset_x;
        s_task_buttons[s_task_btn_count].w = btn_w;
        
        bool focused = (BOS_GetFocus() == win->id);
        uint32_t bg = focused ? 0xFF334155 : 0xFF1E293B;
        
        BWE_FillRect(fb, self->screen_bounds.x + offset_x, self->screen_bounds.y + 10, btn_w, 30, bg);
        
        int32_t max_chars = (btn_w - 20) / 8;
        char disp_text[64];
        task_panel_truncate_text(win->control_data.button.text, disp_text, max_chars);
        
        BWE_DrawText(fb, disp_text, self->screen_bounds.x + offset_x + 10, self->screen_bounds.y + 18, 0xFFF1F5F9, 0);
        
        offset_x += btn_w + 10;
        used_width += btn_w + 10;
        s_task_btn_count++;
    }
    
    g_tp_free_space = available_width > used_width ? available_width - used_width : 0;
    
    if (overflow && active_count > render_count) {
        BWE_DrawText(fb, ">>", self->screen_bounds.x + offset_x, self->screen_bounds.y + 18, 0xFFEAB308, 0);
    }

    // Render Clock on the right
    int hr = 0, min = 0;
    read_rtc_time(&hr, &min);
    char time_str[6] = "00:00";
    format_two_digits(hr, time_str);
    time_str[2] = ':';
    format_two_digits(min, time_str + 3);
    
    int32_t clock_x = self->screen_bounds.width - 60;
    BWE_DrawText(fb, time_str, self->screen_bounds.x + clock_x, self->screen_bounds.y + 18, 0xFF94A3B8, 0);
}

static void task_panel_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x - self->screen_bounds.x;
        int32_t my = event->data.mouse.y - self->screen_bounds.y;

        // Check Start Menu Button
        if (mx >= 10 && mx < 90 && my >= 10 && my < 40) {
            g_start_menu_open = !g_start_menu_open;
            if (g_start_menu_open) {
                BOS_Show(g_start_menu_win_id);
                BOS_SetFocus(g_start_menu_win_id);
            } else {
                BOS_Hide(g_start_menu_win_id);
            }
            BWE_InvalidateWindow(window_id);
            return;
        }

        // Check Task Buttons
        for (uint32_t i = 0; i < s_task_btn_count; i++) {
            if (mx >= s_task_buttons[i].x && mx < s_task_buttons[i].x + s_task_buttons[i].w) {
                if (my >= 10 && my < 40) {
                    BOS_SetFocus(s_task_buttons[i].win_id);
                    // Hide start menu if open
                    if (g_start_menu_open) {
                        g_start_menu_open = false;
                        BOS_Hide(g_start_menu_win_id);
                    }
                    BWE_InvalidateWindow(window_id);
                    return;
                }
            }
        }
    }
}

void TaskPanel_Initialize(void) {
    const AGDAE_Metrics* metrics = AGDAE_GetMetrics();
    
    int32_t px = metrics->taskbar_rect.x;
    int32_t py = metrics->taskbar_rect.y;
    int32_t panel_width = metrics->taskbar_rect.width;
    int32_t panel_height = metrics->taskbar_rect.height;

    BOS_CreatePanel(BWE_DESKTOP_ID, px, py, panel_width, panel_height, 0xFF0F172A, &g_task_panel_win_id);
    BWE_Window* tb = BWE_GetWindow(g_task_panel_win_id);
    if (tb) {
        tb->type = BWE_TYPE_TASKBAR; // Special type so it doesn't get drawn as a regular window
        tb->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST;
        tb->on_render = task_panel_render_callback;
        tb->on_event = task_panel_event_callback;
    }
}

// Hook called by Window Manager
void TaskPanel_Update(void) {
    if (g_task_panel_win_id != 0) {
        BWE_InvalidateWindow(g_task_panel_win_id);
    }
}
