#ifndef BOS_SHELL_PANEL_H
#define BOS_SHELL_PANEL_H

#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/ui/boasset/boasset.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shared BOS System Hub Design System Metrics
#define BOS_METRIC_PANEL_RADIUS       14
#define BOS_METRIC_CARD_RADIUS        8
#define BOS_METRIC_PANEL_PADDING      14
#define BOS_METRIC_SECTION_GAP        10
#define BOS_METRIC_ITEM_GAP           6
#define BOS_METRIC_CAPSULE_HEIGHT     34
#define BOS_METRIC_FLYOUT_GAP         12 // Breathing space between top capsule & flyouts

// Reusable BOS Shell Panel Drawing Primitives
void draw_bos_panel(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, uint32_t bg_color, uint32_t border_color, const BWE_Rect* clip);
void draw_bos_glass_capsule(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, bool is_hovered, const BWE_Rect* clip);
void draw_bos_rounded_box(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, uint32_t color, const BWE_Rect* clip);
void draw_bos_separator(const BVFramebuffer* fb, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color, const BWE_Rect* clip);
void draw_bos_slider(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t percent, bool is_hovered, bool is_dragging, uint32_t accent_color, const BWE_Rect* clip);
void draw_bos_tile(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, const char* title, const char* status_text, uint32_t icon_id, bool is_on, bool is_hovered, bool is_disabled, const BWE_Rect* clip);
void draw_bos_notification_card(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, const char* title, const char* message, const char* time_str, uint32_t app_icon_id, bool is_hovered, const BWE_Rect* clip);

// Procedural System Vector Icons
void draw_vector_wifi(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip);
void draw_vector_volume(const BVFramebuffer* fb, int32_t cx, int32_t cy, bool is_muted, uint32_t color, const BWE_Rect* clip);
void draw_vector_battery(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip);
void draw_vector_bell(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip);
void draw_vector_sun(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip);

#ifdef __cplusplus
}
#endif

#endif // BOS_SHELL_PANEL_H
