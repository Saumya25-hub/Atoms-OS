#include "../../include/bwe.h"

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
    (void)window_id;
    (void)event;
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
