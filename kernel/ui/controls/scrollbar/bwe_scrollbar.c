#include "../../../wm/bwe/include/bwe.h"

static void bwe_scrollbar_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Track scroll channel bg & slider thumb color
    uint32_t track_bg = BWE_ThemeGetColor(BWE_THEME_CONTROL_BG);
    uint32_t thumb_color = BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    if (self->control_data.scrollbar.is_thumb_dragging) {
        thumb_color = BWE_ThemeGetColor(BWE_THEME_SELECTION_BG);
    }

    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, track_bg);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, thumb_color, 1);

    // Compute slider coordinates
    int32_t range = self->control_data.scrollbar.max - self->control_data.scrollbar.min;
    if (range > 0) {
        if (self->control_data.scrollbar.vertical) {
            int32_t thumb_h = self->screen_bounds.height / 5;
            if (thumb_h < 15) thumb_h = 15;

            int32_t travel = self->screen_bounds.height - thumb_h;
            int32_t thumb_y = self->screen_bounds.y + (int32_t)((self->control_data.scrollbar.value - self->control_data.scrollbar.min) * travel / range);

            BWE_FillRect(fb, self->screen_bounds.x + 2, thumb_y, self->screen_bounds.width - 4, thumb_h, thumb_color);
        } else {
            int32_t thumb_w = self->screen_bounds.width / 5;
            if (thumb_w < 15) thumb_w = 15;

            int32_t travel = self->screen_bounds.width - thumb_w;
            int32_t thumb_x = self->screen_bounds.x + (int32_t)((self->control_data.scrollbar.value - self->control_data.scrollbar.min) * travel / range);

            BWE_FillRect(fb, thumb_x, self->screen_bounds.y + 2, thumb_w, self->screen_bounds.height - 4, thumb_color);
        }
    }
}

static void bwe_scrollbar_event(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        self->control_data.scrollbar.is_thumb_dragging = true;
        BWE_InvalidateWindow(window_id);
    } else if (event->type == BWE_EVENT_MOUSE_UP) {
        self->control_data.scrollbar.is_thumb_dragging = false;
        BWE_InvalidateWindow(window_id);
    } else if (event->type == BWE_EVENT_MOUSE_MOVE && self->control_data.scrollbar.is_thumb_dragging) {
        // Compute new value based on drag delta
        int32_t range = self->control_data.scrollbar.max - self->control_data.scrollbar.min;
        if (range > 0) {
            if (self->control_data.scrollbar.vertical) {
                int32_t thumb_h = self->screen_bounds.height / 5;
                if (thumb_h < 15) thumb_h = 15;
                int32_t travel = self->screen_bounds.height - thumb_h;
                int32_t my = event->data.mouse.y - self->screen_bounds.y - (thumb_h / 2);
                if (my < 0) my = 0;
                if (my > travel) my = travel;
                self->control_data.scrollbar.value = self->control_data.scrollbar.min + (my * range / travel);
            } else {
                int32_t thumb_w = self->screen_bounds.width / 5;
                if (thumb_w < 15) thumb_w = 15;
                int32_t travel = self->screen_bounds.width - thumb_w;
                int32_t mx = event->data.mouse.x - self->screen_bounds.x - (thumb_w / 2);
                if (mx < 0) mx = 0;
                if (mx > travel) mx = travel;
                self->control_data.scrollbar.value = self->control_data.scrollbar.min + (mx * range / travel);
            }
            BWE_InvalidateWindow(window_id);
            if (self->control_data.scrollbar.on_scroll) {
                self->control_data.scrollbar.on_scroll(window_id, self->control_data.scrollbar.value);
            }
        }
    }
}

bwe_error_t BOS_CreateScrollBar(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool vertical, int32_t min, int32_t max, void (*on_scroll)(uint32_t, int32_t), uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_SCROLLBAR;
        win->control_data.scrollbar.vertical = vertical;
        win->control_data.scrollbar.min = min;
        win->control_data.scrollbar.max = max;
        win->control_data.scrollbar.value = min;
        win->control_data.scrollbar.is_thumb_dragging = false;
        win->control_data.scrollbar.on_scroll = on_scroll;

        win->on_render = bwe_scrollbar_render;
        win->on_event = bwe_scrollbar_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
