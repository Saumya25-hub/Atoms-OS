#include "../../include/bwe.h"

// Expose internal registry
extern BWE_Window* BWE_GetWindow(uint32_t window_id);

static void bwe_radiobutton_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    int32_t rad_size = 14;
    int32_t rad_x = self->screen_bounds.x;
    int32_t rad_y = self->screen_bounds.y + (self->screen_bounds.height - rad_size) / 2;

    // Fill white box, dark border
    BWE_FillRect(fb, rad_x, rad_y, rad_size, rad_size, 0xFFFFFFFF);
    
    extern uint32_t g_focused_window_id;
    uint32_t border_color = (self->id == g_focused_window_id) ? BWE_ThemeGetColor(BWE_THEME_WINDOW_BORDER_ACTIVE) : BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    BWE_DrawRect(fb, rad_x, rad_y, rad_size, rad_size, border_color, 1);

    if (self->control_data.radiobutton.selected) {
        BWE_FillRect(fb, rad_x + 4, rad_y + 4, rad_size - 8, rad_size - 8, BWE_ThemeGetColor(BWE_THEME_ACCENT));
    }

    int32_t tx = rad_x + rad_size + 8;
    int32_t ty = self->screen_bounds.y + (self->screen_bounds.height - 12) / 2;
    BWE_DrawText(fb, self->control_data.radiobutton.text, tx, ty, BWE_ThemeGetColor(BWE_THEME_TEXT), 0);
}

static void bwe_radiobutton_event(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    if (event->type == BWE_EVENT_MOUSE_DOWN || (event->type == BWE_EVENT_KEY_DOWN && event->data.key.key_code == 0x20)) {
        if (!self->control_data.radiobutton.selected) {
            self->control_data.radiobutton.selected = true;
            BWE_InvalidateWindow(window_id);

            BWE_Window* parent = BWE_GetWindow(self->parent_id);
            if (parent) {
                for (uint32_t i = 0; i < parent->child_count; i++) {
                    uint32_t child_id = parent->children[i];
                    if (child_id != window_id) {
                        BWE_Window* sib = BWE_GetWindow(child_id);
                        if (sib && sib->type == BWE_TYPE_RADIOBUTTON &&
                            sib->control_data.radiobutton.group_id == self->control_data.radiobutton.group_id) {
                            sib->control_data.radiobutton.selected = false;
                            BWE_InvalidateWindow(child_id);
                        }
                    }
                }
            }

            if (self->control_data.radiobutton.on_select) {
                self->control_data.radiobutton.on_select(window_id);
            }
        }
    }
}

bwe_error_t BOS_CreateRadioButton(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, uint32_t group_id, void (*on_select)(uint32_t), uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS | BWE_WINDOW_TRANSPARENT, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_RADIOBUTTON;
        win->control_data.radiobutton.selected = false;
        win->control_data.radiobutton.group_id = group_id;
        win->control_data.radiobutton.on_select = on_select;

        if (text) {
            uint32_t i = 0;
            for (; text[i] != '\0' && i < 127; i++) {
                win->control_data.radiobutton.text[i] = text[i];
            }
            win->control_data.radiobutton.text[i] = '\0';
        } else {
            win->control_data.radiobutton.text[0] = '\0';
        }

        win->on_render = bwe_radiobutton_render;
        win->on_event = bwe_radiobutton_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
