#include "../../../wm/bwe/include/bwe.h"

static void bwe_label_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    if (!self->control_data.label.transparent) {
        uint32_t bg = BWE_ThemeGetColor(BWE_THEME_WINDOW_BG);
        BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, bg);
    }

    BWE_DrawText(fb, self->control_data.label.text, self->screen_bounds.x, self->screen_bounds.y, self->control_data.label.text_color, 0);
}

static void bwe_label_event(uint32_t window_id, const BWE_Event* event) {
    (void)window_id;
    (void)event;
}

bwe_error_t BOS_CreateLabel(uint32_t parent_id, uint32_t x, uint32_t y, const char* text, uint32_t color_fg, uint32_t* out_id) {
    uint32_t id = 0;
    
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, 200, 16, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TRANSPARENT, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_LABEL;
        win->control_data.label.text_color = color_fg;
        win->control_data.label.transparent = true;

        if (text) {
            uint32_t i = 0;
            for (; text[i] != '\0' && i < 127; i++) {
                win->control_data.label.text[i] = text[i];
            }
            win->control_data.label.text[i] = '\0';
        } else {
            win->control_data.label.text[0] = '\0';
        }

        win->on_render = bwe_label_render;
        win->on_event = bwe_label_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
