#ifndef GUI_THEME_ENGINE_H
#define GUI_THEME_ENGINE_H

#include "kernel/gui/surface/surface.h"
#include "kernel/gui/render/painter.h"
#include "bovisual/Include/bovisual_types.h"

// Standard sizes
#define THEME_TITLEBAR_HEIGHT 24
#define THEME_BORDER_WIDTH    2
#define THEME_BUTTON_SIZE     16

// GUI Controls Palettes
BOVISUAL_Color theme_get_control_bg(bool is_focused, bool is_hovered, bool is_pressed);
BOVISUAL_Color theme_get_control_fg(bool is_disabled);
BOVISUAL_Color theme_get_control_border(bool is_focused);

// Draw the full window decoration (titlebar, borders)
void theme_draw_window_frame(struct BOSSurface* surface, const char* title, bool is_focused, const BVRect* clip);

// Draw the client area background (optional if application draws it)
void theme_draw_client_background(struct BOSSurface* surface, bool is_focused, const BVRect* clip);

// Shell Rendering APIs
void theme_draw_taskbar(struct BOSSurface* surface, const BVRect* clip);
void theme_draw_start_button(struct BOSSurface* surface, int x, int y, int width, int height, int state, const BVRect* clip);
void theme_draw_desktop(struct BOSSurface* surface, const BVRect* clip);
void theme_draw_icon(struct BOSSurface* surface, int x, int y, const char* label, int icon_id, bool is_selected, bool is_hovered, const BVRect* clip);

// Start Menu Rendering APIs
void theme_draw_start_menu(struct BOSSurface* surface, const BVRect* clip);
void theme_draw_menu_item(struct BOSSurface* surface, int x, int y, int width, int height, const char* label, int icon_id, bool is_hovered, const BVRect* clip);

#endif // GUI_THEME_ENGINE_H
