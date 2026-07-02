#include "../../../wm/bwe/include/bwe.h"

static void bwe_progressbar_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Track frame border & background
    uint32_t bg = BWE_ThemeGetColor(BWE_THEME_CONTROL_BG);
    uint32_t border = BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    uint32_t fill = BWE_ThemeGetColor(BWE_THEME_ACCENT);

    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, bg);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, border, 1);

    // Compute progress width
    int32_t range = self->control_data.progressbar.max - self->control_data.progressbar.min;
    if (range > 0) {
        int32_t fill_w = (int32_t)((self->control_data.progressbar.value - self->control_data.progressbar.min) * (self->screen_bounds.width - 4) / range);
        if (fill_w > 0) {
            BWE_FillRect(fb, self->screen_bounds.x + 2, self->screen_bounds.y + 2, fill_w, self->screen_bounds.height - 4, fill);
        }
    }
}

static void bwe_progressbar_event(uint32_t window_id, const BWE_Event* event) {
    (void)window_id;
    (void)event;
}

bwe_error_t BOS_CreateProgressBar(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, int32_t min, int32_t max, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_PROGRESSBAR;
        win->control_data.progressbar.min = min;
        win->control_data.progressbar.max = max;
        win->control_data.progressbar.value = min;
        win->on_render = bwe_progressbar_render;
        win->on_event = bwe_progressbar_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
