/**
 * @file agdae_geometry.c
 * @brief ATOMS OS Display Adaptation Engine - Geometry Pipeline
 */

#include "agdae.h"

extern AGDAE_Metrics g_agdae_metrics;

void AGDAE_Geometry_Update(AGDAE_Metrics* metrics) {
    if (!metrics) return;

    /* Base logical resolution to work with */
    int32_t w = (int32_t)metrics->logical_width;
    int32_t h = (int32_t)metrics->logical_height;

    /* 1. Base Taskbar Height (Unscaled) */
    int32_t base_taskbar_height = (h <= 600) ? 40 : 48;
    
    /* Apply Scaling */
    int32_t taskbar_height = AGDAE_Scale(base_taskbar_height);

    /* 2. Desktop Rect (Logical bounds for UI components, centered if letterboxed) */
    int32_t offset_x = 0;
    int32_t offset_y = 0;
    if (metrics->is_letterboxed) {
        offset_x = ((int32_t)metrics->physical_width - w) / 2;
        offset_y = ((int32_t)metrics->physical_height - h) / 2;
    }
    
    metrics->desktop_rect.x = offset_x;
    metrics->desktop_rect.y = offset_y;
    metrics->desktop_rect.width = w;
    metrics->desktop_rect.height = h;

    /* 3. Wallpaper Rect (Spans the full physical framebuffer) */
    /* This ensures even if logical is smaller, wallpaper covers the black borders */
    metrics->wallpaper_rect.x = 0;
    metrics->wallpaper_rect.y = 0;
    metrics->wallpaper_rect.width = (int32_t)metrics->physical_width;
    metrics->wallpaper_rect.height = (int32_t)metrics->physical_height;

    /* 4. Taskbar Rect (Floating centered design from TaskPanel, or full width) */
    /* Wait, the user asked for "Bottom center. Floating panel" in previous phases, but let's standardise it here */
    int32_t panel_w = AGDAE_Scale(800);
    if (panel_w > w) panel_w = w;
    metrics->taskbar_rect.width = panel_w;
    metrics->taskbar_rect.height = taskbar_height;
    metrics->taskbar_rect.x = (w - panel_w) / 2;
    metrics->taskbar_rect.y = h - taskbar_height;

    /* 5. Safe Area */
    /* Guaranteed visible region. We inset by 16 scaled pixels from logical edges */
    int32_t margin = AGDAE_Scale(16);
    metrics->safe_area.x = margin;
    metrics->safe_area.y = margin;
    metrics->safe_area.width = (w > margin * 2) ? (w - margin * 2) : w;
    metrics->safe_area.height = (h > taskbar_height + margin * 2) ? (h - taskbar_height - margin * 2) : h;

    /* 6. Window Work Area (Authoritative desktop workspace for maximized windows) */
    metrics->window_work_area.x = 0;
    metrics->window_work_area.y = 0;
    metrics->window_work_area.width = w;
    metrics->window_work_area.height = h - taskbar_height;

    /* 7. Notification Area (Top Right) */
    int32_t notif_base_w = 304;
    int32_t notif_w = AGDAE_Scale(notif_base_w);
    if (notif_w > w - margin * 2) notif_w = w - margin * 2;
    metrics->notification_area.width = notif_w;
    metrics->notification_area.height = AGDAE_Scale(400);
    if (metrics->notification_area.height > h - margin * 2) metrics->notification_area.height = h - margin * 2;
    metrics->notification_area.x = w - notif_w - margin;
    metrics->notification_area.y = margin;
}
