/**
 * @file display_geometry.c
 * @brief ATOMS OS Display Intelligence Engine - Geometry Authority Implementation
 */

#include "display_geometry.h"

void DIE_Geometry_Calculate(DIE_DisplayInfo* info) {
    if (!info) return;

    uint32_t w = info->active_mode.width;
    uint32_t h = info->active_mode.height;
    if (w == 0) w = 1280;
    if (h == 0) h = 720;

    /* Standard bottom taskbar height in ATOMS OS V2 UI */
    uint32_t taskbar_height = 48;
    if (h <= 600) taskbar_height = 40;

    /* 1. Desktop Rect (Full screen bounding box) */
    info->geometry.desktop_rect.x = 0;
    info->geometry.desktop_rect.y = 0;
    info->geometry.desktop_rect.width = w;
    info->geometry.desktop_rect.height = h;

    /* 2. Wallpaper Rect (Full screen background fill area) */
    info->geometry.wallpaper_rect.x = 0;
    info->geometry.wallpaper_rect.y = 0;
    info->geometry.wallpaper_rect.width = w;
    info->geometry.wallpaper_rect.height = h;

    /* 3. Taskbar Rect (Bottom panel anchored horizontally across width) */
    info->geometry.taskbar_rect.x = 0;
    info->geometry.taskbar_rect.y = (int32_t)(h - taskbar_height);
    info->geometry.taskbar_rect.width = w;
    info->geometry.taskbar_rect.height = taskbar_height;

    /* 4. Notification Area (Top-right sliding alert stack) */
    uint32_t notif_w = (w > 340) ? 304 : (w - 36);
    info->geometry.notification_area.x = (int32_t)(w - notif_w - 16);
    info->geometry.notification_area.y = 16;
    info->geometry.notification_area.width = notif_w;
    info->geometry.notification_area.height = (h > 450) ? 400 : (h - 60);

    /* 5. Popup Area (Centered modal / dialog overlay box) */
    uint32_t popup_w = (w > 450) ? 400 : (w - 50);
    uint32_t popup_h = (h > 350) ? 300 : (h - 50);
    info->geometry.popup_area.x = (int32_t)((w - popup_w) / 2);
    info->geometry.popup_area.y = (int32_t)((h - popup_h) / 2);
    info->geometry.popup_area.width = popup_w;
    info->geometry.popup_area.height = popup_h;

    /* 6. Window Work Area (Authoritative desktop workspace above taskbar for maximized windows) */
    info->geometry.window_work_area.x = 0;
    info->geometry.window_work_area.y = 0;
    info->geometry.window_work_area.width = w;
    info->geometry.window_work_area.height = (h > taskbar_height) ? (h - taskbar_height) : h;

    /* 7. Cursor Bounds (Valid 0-indexed coordinate domain for Pointer/Cursor Engine) */
    info->geometry.cursor_bounds.x = 0;
    info->geometry.cursor_bounds.y = 0;
    info->geometry.cursor_bounds.width = (w > 0) ? (w - 1) : 0;
    info->geometry.cursor_bounds.height = (h > 0) ? (h - 1) : 0;

    /* 8. Safe Area (Screen inset margin protected from bezel overscan or display edge clipping) */
    info->geometry.safe_area.x = 8;
    info->geometry.safe_area.y = 8;
    info->geometry.safe_area.width = (w > 16) ? (w - 16) : w;
    info->geometry.safe_area.height = (h > taskbar_height + 16) ? (h - taskbar_height - 16) : h;

    /* 9. Dock Area (Centered bottom floating dock alternative placement) */
    uint32_t dock_w = (w > 550) ? 500 : (w - 32);
    info->geometry.dock_area.x = (int32_t)((w - dock_w) / 2);
    info->geometry.dock_area.y = (int32_t)(h - taskbar_height - 6);
    info->geometry.dock_area.width = dock_w;
    info->geometry.dock_area.height = taskbar_height;
}
