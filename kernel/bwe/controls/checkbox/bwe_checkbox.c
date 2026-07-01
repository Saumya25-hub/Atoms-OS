#include "../../include/bwe.h"

static void bwe_checkbox_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
    extern void BWE_DrawLine(const BVFramebuffer* fb, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    int32_t box_size = 16;
    int32_t box_x = self->screen_bounds.x;
    int32_t box_y = self->screen_bounds.y + (self->screen_bounds.height - box_size) / 2;

    // White box, dark border
    BWE_FillRect(fb, box_x, box_y, box_size, box_size, 0xFFFFFFFF);
    
    extern uint32_t g_focused_window_id;
    uint32_t border_color = (self->id == g_focused_window_id) ? BWE_ThemeGetColor(BWE_THEME_WINDOW_BORDER_ACTIVE) : BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    BWE_DrawRect(fb, box_x, box_y, box_size, box_size, border_color, 1);

    if (self->control_data.checkbox.checked) {
        BWE_DrawLine(fb, box_x + 3, box_y + 3, box_x + box_size - 4, box_y + box_size - 4, 0xFFEF4444);
        BWE_DrawLine(fb, box_x + box_size - 4, box_y + 3, box_x + 3, box_y + box_size - 4, 0xFFEF4444);
    }

    int32_t tx = box_x + box_size + 8;
    int32_t ty = self->screen_bounds.y + (self->screen_bounds.height - 12) / 2;
    BWE_DrawText(fb, self->control_data.checkbox.text, tx, ty, BWE_ThemeGetColor(BWE_THEME_TEXT), 0);
}

static void bwe_checkbox_event(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        self->control_data.checkbox.checked = !self->control_data.checkbox.checked;
        BWE_InvalidateWindow(window_id);
        if (self->control_data.checkbox.on_toggle) {
            self->control_data.checkbox.on_toggle(window_id, self->control_data.checkbox.checked);
        }
    } else if (event->type == BWE_EVENT_KEY_DOWN) {
        if (event->data.key.key_code == 0x20) { // Spacebar toggle
            self->control_data.checkbox.checked = !self->control_data.checkbox.checked;
            BWE_InvalidateWindow(window_id);
            if (self->control_data.checkbox.on_toggle) {
                self->control_data.checkbox.on_toggle(window_id, self->control_data.checkbox.checked);
            }
        }
    }
}

bwe_error_t BOS_CreateCheckbox(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, void (*on_toggle)(uint32_t, bool), uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TRANSPARENT, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_CHECKBOX;
        win->control_data.checkbox.checked = false;
        win->control_data.checkbox.on_toggle = on_toggle;

        if (text) {
            uint32_t i = 0;
            for (; text[i] != '\0' && i < 127; i++) {
                win->control_data.checkbox.text[i] = text[i];
            }
            win->control_data.checkbox.text[i] = '\0';
        } else {
            win->control_data.checkbox.text[0] = '\0';
        }

        win->on_render = bwe_checkbox_render;
        win->on_event = bwe_checkbox_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
