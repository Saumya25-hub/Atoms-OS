#include "desktop_shell.h"
#include "../lib/include/string.h"

// Screen Resolution
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

// External Globals
extern uint32_t g_taskbar_win_id;
extern uint32_t g_start_menu_win_id;
extern bool g_start_menu_open;
extern BWE_Window g_windows[];
extern uint32_t g_active_window_id;

// Telemetry counters
extern uint32_t g_hud_taskbar_buttons;

// Port IO declarations
extern void io_out8(uint16_t port, uint8_t data);
extern uint8_t io_in8(uint16_t port);

// RTC Time Retrieval
static uint8_t rtc_read(uint8_t reg) {
    io_out8(0x70, reg);
    return io_in8(0x71);
}

static void read_rtc_time(int* hour, int* minute, int* second) {
    // Wait until RTC update is not in progress
    int timeout = 1000;
    while (timeout-- > 0) {
        io_out8(0x70, 0x0A);
        if (!(io_in8(0x71) & 0x80)) break;
    }
    
    uint8_t sec = rtc_read(0x00);
    uint8_t min = rtc_read(0x02);
    uint8_t hr  = rtc_read(0x04);
    uint8_t regB = rtc_read(0x0B);
    
    // BCD conversion
    if (!(regB & 0x04)) {
        sec = ((sec & 0xF0) >> 4) * 10 + (sec & 0x0F);
        min = ((min & 0xF0) >> 4) * 10 + (min & 0x0F);
        hr  = ((hr & 0xF0) >> 4) * 10 + (hr & 0x0F);
    }
    
    // 12-hour AM/PM to 24-hour conversion
    if (!(regB & 0x02) && (hr & 0x80)) {
        hr = ((hr & 0x7F) + 12) % 24;
    }
    
    *hour = hr;
    *minute = min;
    *second = sec;
}

// Helper to format integers as 2 digits (e.g. 9 -> "09")
static void format_two_digits(int val, char* out) {
    if (val < 0) val = 0;
    if (val > 99) val = 99;
    out[0] = (val / 10) + '0';
    out[1] = (val % 10) + '0';
    out[2] = '\0';
}

// Running window button structure for mapping click positions
#define MAX_TASKBAR_BUTTONS 10
typedef struct {
    uint32_t win_id;
    int32_t x;
    int32_t w;
} TaskbarBtn;
static TaskbarBtn s_taskbar_buttons[MAX_TASKBAR_BUTTONS];
static uint32_t s_taskbar_btn_count = 0;

void taskbar_update_windows_list(void);
static void start_menu_render_callback(BWE_Window* self);
static void start_menu_event_callback(uint32_t window_id, const BWE_Event* event);
static void taskbar_render_callback(BWE_Window* self);
static void taskbar_event_callback(uint32_t window_id, const BWE_Event* event);

// Initialize Taskbar and Start Menu
void taskbar_initialize(void) {
    int32_t sw = (int32_t)g_kernel_screen_width;
    int32_t sh = (int32_t)g_kernel_screen_height;
    
    // Create Taskbar Dock Panel
    BOS_CreatePanel(BWE_DESKTOP_ID, 0, sh - 48, sw, 48, 0xFF1E293B, &g_taskbar_win_id);
    BWE_Window* tb = BWE_GetWindow(g_taskbar_win_id);
    if (tb) {
        tb->type = BWE_TYPE_TASKBAR;
        tb->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST;
        tb->on_render = taskbar_render_callback;
        tb->on_event = taskbar_event_callback;
    }
    
    // Create Start Menu Popup Panel (Hidden by default)
    BOS_CreatePanel(BWE_DESKTOP_ID, 5, sh - 48 - 240, 260, 240, 0xFF0F172A, &g_start_menu_win_id);
    BWE_Window* sm = BWE_GetWindow(g_start_menu_win_id);
    if (sm) {
        sm->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST;
        sm->on_render = start_menu_render_callback;
        sm->on_event = start_menu_event_callback;
        BOS_Hide(g_start_menu_win_id);
    }
    g_start_menu_open = false;
    
    taskbar_update_windows_list();
}

// Telemetry window count and taskbar synchronization
void taskbar_update_windows_list(void) {
    s_taskbar_btn_count = 0;
    for (uint32_t i = 0; i < BWE_MAX_WINDOWS; i++) {
        BWE_Window* win = &g_windows[i];
        if (win->state != BWE_STATE_DESTROYED && win->parent_id == BWE_DESKTOP_ID && win->id != BWE_DESKTOP_ID && win->id != g_taskbar_win_id && win->id != g_start_menu_win_id && win->type == BWE_TYPE_WINDOW) {
            if (s_taskbar_btn_count < MAX_TASKBAR_BUTTONS) {
                s_taskbar_buttons[s_taskbar_btn_count].win_id = win->id;
                s_taskbar_buttons[s_taskbar_btn_count].x = 90 + s_taskbar_btn_count * 135;
                s_taskbar_buttons[s_taskbar_btn_count].w = 130;
                s_taskbar_btn_count++;
            }
        }
    }
    g_hud_taskbar_buttons = s_taskbar_btn_count;
    BWE_InvalidateWindow(g_taskbar_win_id);
}

// Periodic tick updates (for the Clock second tracker)
void taskbar_pulse(void) {
    static uint32_t s_last_pulse_sec = 99;
    int h, m, s;
    read_rtc_time(&h, &m, &s);
    if (s != (int)s_last_pulse_sec) {
        s_last_pulse_sec = s;
        BWE_InvalidateWindow(g_taskbar_win_id);
    }
}

// Procedural Start Menu items
static void start_menu_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    BWE_Rect b = self->screen_bounds;
    
    // Fill Slate-900 panel background
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF0F172A);
    BWE_DrawRect(fb, b.x, b.y, b.width, b.height, 0xFF3B82F6, 2); // Blue accent outline
    
    // Draw Start Menu title banner
    BWE_FillRect(fb, b.x + 2, b.y + 2, b.width - 4, 30, 0xFF1E293B);
    BWE_DrawText(fb, "Applications Menu", b.x + 12, b.y + 10, 0xFFE2E8F0, 0);
    
    // Draw registered items
    uint32_t count = Shell_GetAppCount();
    for (uint32_t i = 0; i < count; i++) {
        ShellAppEntry* app = Shell_GetAppEntry(i);
        if (!app) continue;
        
        int32_t iy = b.y + 36 + i * 40;
        
        // Draw item hover highlight (if mouse is over this slot)
        extern int32_t g_bwe_mouse_x;
        extern int32_t g_bwe_mouse_y;
        bool is_hover = (g_bwe_mouse_x >= b.x && g_bwe_mouse_x < b.x + b.width &&
                         g_bwe_mouse_y >= iy && g_bwe_mouse_y < iy + 38);
        if (is_hover) {
            BWE_FillRect(fb, b.x + 4, iy, b.width - 8, 36, 0x443B82F6);
        }
        
        // Draw icon indicator box
        BWE_FillRect(fb, b.x + 10, iy + 6, 24, 24, 0xFF334155);
        BWE_DrawRect(fb, b.x + 10, iy + 6, 24, 24, 0xFF475569, 1);
        
        // Simple letter logo for apps
        char icon_char[2] = { app->display_name[0], '\0' };
        BWE_DrawText(fb, icon_char, b.x + 18, iy + 10, 0xFF3B82F6, 0);
        
        // Display App Name
        BWE_DrawText(fb, app->display_name, b.x + 45, iy + 10, 0xFFF1F5F9, 0);
    }
}

static void start_menu_event_callback(uint32_t window_id, const BWE_Event* event) {
    (void)window_id;
    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        BWE_Window* self = BWE_GetWindow(g_start_menu_win_id);
        if (!self) return;
        
        BWE_Rect b = self->screen_bounds;
        int32_t my = event->data.mouse.y;
        
        int32_t slot = (my - b.y - 36) / 40;
        uint32_t count = Shell_GetAppCount();
        if (slot >= 0 && (uint32_t)slot < count) {
            ShellAppEntry* app = Shell_GetAppEntry(slot);
            if (app) {
                // Launch
                uint32_t new_win;
                Shell_LaunchApp(app->app_id, &new_win);
                
                // Hide menu
                g_start_menu_open = false;
                BOS_Hide(g_start_menu_win_id);
                BWE_InvalidateWindow(BWE_DESKTOP_ID);
            }
        }
    }
}

// Procedural Taskbar rendering
static void taskbar_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    BWE_Rect b = self->screen_bounds;
    
    // Fill Taskbar background (Dark Slate panel)
    BWE_FillRect(fb, b.x, b.y, b.width, b.height, 0xFF1E293B);
    BWE_FillRect(fb, b.x, b.y, b.width, 2, 0xFF475569); // Top edge line border
    
    // 1. Draw Start Button (Left side)
    int32_t start_hover = 0;
    extern int32_t g_bwe_mouse_x;
    extern int32_t g_bwe_mouse_y;
    if (g_bwe_mouse_x >= b.x + 10 && g_bwe_mouse_x < b.x + 80 &&
        g_bwe_mouse_y >= b.y + 8 && g_bwe_mouse_y < b.y + 40) {
        start_hover = 1;
    }
    
    uint32_t start_color = g_start_menu_open ? 0xFF059669 : (start_hover ? 0xFF10B981 : 0xFF047857);
    BWE_FillRect(fb, b.x + 10, b.y + 8, 70, 32, start_color);
    BWE_DrawRect(fb, b.x + 10, b.y + 8, 70, 32, 0xFF065F46, 1);
    BWE_DrawText(fb, "START", b.x + 25, b.y + 16, 0xFFFFFFFF, 0);
    
    // 2. Draw System Tray Icons & Clock (Right side)
    // Draw Clock
    int hour, minute, second;
    read_rtc_time(&hour, &minute, &second);
    char clock_str[16];
    char temp[4];
    format_two_digits(hour, temp);
    strcpy(clock_str, temp);
    strcat(clock_str, ":");
    format_two_digits(minute, temp);
    strcat(clock_str, temp);
    strcat(clock_str, ":");
    format_two_digits(second, temp);
    strcat(clock_str, temp);
    
    BWE_DrawText(fb, clock_str, b.x + b.width - 90, b.y + 16, 0xFFFFFFFF, 0);
    
    // Draw Volume Icon (Soundwave procedural lines)
    int32_t vx = b.x + b.width - 150;
    int32_t vy = b.y + 16;
    BWE_FillRect(fb, vx, vy + 4, 4, 8, 0xFF94A3B8); // speaker base
    BWE_FillRect(fb, vx + 4, vy + 2, 4, 12, 0xFF94A3B8); // cone
    BWE_FillRect(fb, vx + 12, vy + 2, 2, 12, 0xFF94A3B8); // wave 1
    BWE_FillRect(fb, vx + 16, vy, 2, 16, 0xFF94A3B8); // wave 2
    
    // Draw Network Icon (4 bars)
    int32_t nx = b.x + b.width - 180;
    int32_t ny = b.y + 16;
    BWE_FillRect(fb, nx, ny + 12, 3, 4, 0xFF10B981);
    BWE_FillRect(fb, nx + 5, ny + 8, 3, 8, 0xFF10B981);
    BWE_FillRect(fb, nx + 10, ny + 4, 3, 12, 0xFF10B981);
    BWE_FillRect(fb, nx + 15, ny, 3, 16, 0xFF10B981);
    
    // 3. Draw Running Windows Buttons
    for (uint32_t i = 0; i < s_taskbar_btn_count; i++) {
        TaskbarBtn* btn = &s_taskbar_buttons[i];
        BWE_Window* w = BWE_GetWindow(btn->win_id);
        if (!w) continue;
        
        bool is_active = (btn->win_id == g_active_window_id);
        bool is_minimized = (w->state == BWE_STATE_HIDDEN);
        
        // Hover checking
        bool is_hover = (g_bwe_mouse_x >= b.x + btn->x && g_bwe_mouse_x < b.x + btn->x + btn->w &&
                         g_bwe_mouse_y >= b.y + 8 && g_bwe_mouse_y < b.y + 40);
                         
        uint32_t btn_color = is_active ? 0xFF3B82F6 : (is_hover ? 0xFF475569 : 0xFF334155);
        if (is_minimized) {
            btn_color = is_hover ? 0xFF1E293B : 0xFF0F172A; // Darker if minimized
        }
        
        BWE_FillRect(fb, b.x + btn->x, b.y + 8, btn->w, 32, btn_color);
        BWE_DrawRect(fb, b.x + btn->x, b.y + 8, btn->w, 32, 0xFF475569, 1);
        
        // Draw window title (clip to fit button width)
        char title_clip[14];
        strncpy(title_clip, w->control_data.button.text, 12);
        title_clip[12] = '\0';
        if (strlen(w->control_data.button.text) > 12) {
            title_clip[10] = '.'; title_clip[11] = '.'; title_clip[12] = '\0';
        }
        
        uint32_t text_color = is_minimized ? 0xFF64748B : 0xFFFFFFFF;
        BWE_DrawText(fb, title_clip, b.x + btn->x + 8, b.y + 16, text_color, 0);
        
        // Draw active bottom line indicator
        if (is_active) {
            BWE_FillRect(fb, b.x + btn->x + 4, b.y + 36, btn->w - 8, 2, 0xFF60A5FA);
        }
    }
}

static void taskbar_event_callback(uint32_t window_id, const BWE_Event* event) {
    (void)window_id;
    BWE_Window* self = BWE_GetWindow(g_taskbar_win_id);
    if (!self) return;
    
    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;
        
        BWE_Rect b = self->screen_bounds;
        
        // 1. Click Start Button
        if (mx >= b.x + 10 && mx < b.x + 80) {
            g_start_menu_open = !g_start_menu_open;
            if (g_start_menu_open) {
                BOS_Show(g_start_menu_win_id);
                BWE_BringToFront(g_start_menu_win_id);
                BOS_SetFocus(g_start_menu_win_id);
            } else {
                BOS_Hide(g_start_menu_win_id);
            }
            BWE_InvalidateWindow(g_taskbar_win_id);
            BWE_InvalidateWindow(BWE_DESKTOP_ID);
            return;
        }
        
        // 2. Click App Button
        for (uint32_t i = 0; i < s_taskbar_btn_count; i++) {
            TaskbarBtn* btn = &s_taskbar_buttons[i];
            if (mx >= b.x + btn->x && mx < b.x + btn->x + btn->w) {
                uint32_t win_id = btn->win_id;
                BWE_Window* w = BWE_GetWindow(win_id);
                if (!w) continue;
                
                if (win_id == g_active_window_id) {
                    // Active -> Minimize it
                    BOS_Hide(win_id);
                    BOS_ClearFocus();
                    BWE_UpdateZOrders();
                } else {
                    // Hidden/Inactive -> Focus/Restore
                    if (w->state == BWE_STATE_HIDDEN) {
                        BOS_Show(win_id);
                    }
                    BWE_BringToFront(win_id);
                    BOS_SetFocus(win_id);
                }
                
                // Toggle off Start Menu
                if (g_start_menu_open) {
                    g_start_menu_open = false;
                    BOS_Hide(g_start_menu_win_id);
                }
                
                BWE_InvalidateWindow(BWE_DESKTOP_ID);
                return;
            }
        }
    }
}
