#include "start_menu.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/engine/horse_engine.h"
#include "kernel/core/lib/include/string.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

uint32_t g_start_menu_win_id = 0;
bool g_start_menu_open = false;

static void start_menu_render_callback(BWE_Window* self) {
    extern const BVFramebuffer* BWE_GetRenderTarget(void);
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Outer Background
    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, 0xFF0F172A);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, 0xFF334155, 1);

    // Right side system panel
    int32_t rx = self->screen_bounds.x + 400;
    BWE_FillRect(fb, rx, self->screen_bounds.y, 200, self->screen_bounds.height, 0xFF1E293B);

    // Left side: Pinned Apps
    BWE_DrawText(fb, "Pinned", self->screen_bounds.x + 20, self->screen_bounds.y + 20, 0xFF94A3B8, 0);

    BWE_FillRect(fb, self->screen_bounds.x + 20, self->screen_bounds.y + 60, 360, 40, 0xFF1E293B);
    BWE_DrawText(fb, "ATOMS Search", self->screen_bounds.x + 40, self->screen_bounds.y + 72, 0xFFF1F5F9, 0);

    BWE_FillRect(fb, self->screen_bounds.x + 20, self->screen_bounds.y + 110, 360, 40, 0xFF1E293B);
    BWE_DrawText(fb, "File Explorer", self->screen_bounds.x + 40, self->screen_bounds.y + 122, 0xFFF1F5F9, 0);

    BWE_FillRect(fb, self->screen_bounds.x + 20, self->screen_bounds.y + 160, 360, 40, 0xFF1E293B);
    BWE_DrawText(fb, "Terminal", self->screen_bounds.x + 40, self->screen_bounds.y + 172, 0xFFF1F5F9, 0);

    BWE_FillRect(fb, self->screen_bounds.x + 20, self->screen_bounds.y + 210, 360, 40, 0xFF1E293B);
    BWE_DrawText(fb, "Calculator", self->screen_bounds.x + 40, self->screen_bounds.y + 222, 0xFFF1F5F9, 0);

    BWE_FillRect(fb, self->screen_bounds.x + 20, self->screen_bounds.y + 260, 360, 40, 0xFF1E293B);
    BWE_DrawText(fb, "Settings", self->screen_bounds.x + 40, self->screen_bounds.y + 272, 0xFFF1F5F9, 0);

    // Right Side Profile & System options
    BWE_FillRect(fb, rx + (200-50)/2, self->screen_bounds.y + 40, 50, 50, 0xFF2563EB); // User Icon
    BWE_DrawText(fb, "ATOMS", rx + (200-40)/2, self->screen_bounds.y + 100, 0xFFF1F5F9, 0); // User Name

    BWE_DrawRect(fb, rx + 20, self->screen_bounds.y + 130, 160, 1, 0xFF334155, 1); // Divider

    BWE_DrawText(fb, "Settings", rx + 40, self->screen_bounds.y + 160, 0xFFF1F5F9, 0);
    BWE_DrawText(fb, "Restart",  rx + 40, self->screen_bounds.y + 210, 0xFFF1F5F9, 0);
    BWE_DrawText(fb, "Power Off",rx + 40, self->screen_bounds.y + 260, 0xFFF1F5F9, 0);
}

static void start_menu_event_callback(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x - self->screen_bounds.x;
        int32_t my = event->data.mouse.y - self->screen_bounds.y;

        if (mx >= 20 && mx <= 380) {
            if (my >= 60 && my <= 100) {
                horse_launch(APP_ID_ATOMS);
            } else if (my >= 110 && my <= 150) {
                horse_launch(APP_ID_EXPLORER);
            } else if (my >= 160 && my <= 200) {
                horse_launch(APP_ID_TERMINAL);
            } else if (my >= 210 && my <= 250) {
                horse_launch(APP_ID_CALCULATOR);
            } else if (my >= 260 && my <= 300) {
                horse_launch(APP_ID_SETTINGS);
            }
        }

        if (mx >= 400 && mx <= 600) {
            if (my >= 150 && my <= 180) {
                horse_launch(APP_ID_SETTINGS);
            } else if (my >= 200 && my <= 230) {
                horse_restart();
            } else if (my >= 250 && my <= 280) {
                horse_shutdown();
            }
        }

        // Close after click
        g_start_menu_open = false;
        BOS_Hide(g_start_menu_win_id);
        extern uint32_t g_task_panel_win_id;
        if (g_task_panel_win_id) BWE_InvalidateWindow(g_task_panel_win_id);
    }
}

void StartMenu_Initialize(void) {
    int32_t sw = (int32_t)g_kernel_screen_width;
    int32_t sh = (int32_t)g_kernel_screen_height;
    
    int32_t panel_width = 800;
    if (panel_width > sw) panel_width = sw;
    int32_t px = (sw - panel_width) / 2;
    int32_t py = sh - 50;

    int32_t start_w = 600;
    int32_t start_h = 320;
    int32_t start_x = px;
    int32_t start_y = py - start_h - 10;

    BOS_CreatePanel(BWE_DESKTOP_ID, start_x, start_y, start_w, start_h, 0xFF0F172A, &g_start_menu_win_id);
    BWE_Window* sm = BWE_GetWindow(g_start_menu_win_id);
    if (sm) {
        sm->type = BWE_TYPE_PANEL;
        sm->flags = BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TOPMOST;
        sm->on_render = start_menu_render_callback;
        sm->on_event = start_menu_event_callback;
        BOS_Hide(g_start_menu_win_id);
        g_start_menu_open = false;
    }
}
