#include "../../../wm/bwe/include/bwe.h"

static void bwe_panel_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Fill background
    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, self->control_data.panel.bg_color);
    
    // Draw thin border around panel
    uint32_t border_color = BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, border_color, 1);
}

static void bwe_panel_event(uint32_t window_id, const BWE_Event* event) {
    /* Panels are non-interactive containers. If a mouse button event lands on
     * a panel (e.g. in the gap between buttons), propagate it up the parent
     * chain so the first ancestor that can handle it receives the event.
     * This prevents silent event burial that made buttons unreachable. */
    if (event->type != BWE_EVENT_MOUSE_DOWN && event->type != BWE_EVENT_MOUSE_UP) {
        return; /* Only propagate clicks, not move/enter/leave */
    }

    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    uint32_t parent_id = self->parent_id;
    while (parent_id != 0 && parent_id != BWE_DESKTOP_ID) {
        BWE_Window* parent = BWE_GetWindow(parent_id);
        if (!parent) break;
        if (parent->on_event && parent->type != BWE_TYPE_PANEL) {
            /* Found a non-panel ancestor with an event handler — deliver there */
            BWE_Event fwd = *event;
            fwd.target_id = parent_id;
            parent->on_event(parent_id, &fwd);
            return;
        }
        parent_id = parent->parent_id;
    }
}


bwe_error_t BOS_CreatePanel(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color_bg, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_PANEL;
        win->control_data.panel.bg_color = color_bg;
        win->on_render = bwe_panel_render;
        win->on_event = bwe_panel_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
