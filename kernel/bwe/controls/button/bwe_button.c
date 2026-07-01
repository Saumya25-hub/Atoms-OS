#include "../../include/bwe.h"

static void bwe_button_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Resolve color tokens based on interactive state
    uint32_t bg = self->control_data.button.bg_color;
    uint32_t fg = self->control_data.button.text_color;
    uint32_t border = BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);

    if (self->control_data.button.is_pressed) {
        bg = BWE_ThemeGetColor(BWE_THEME_SELECTION_BG);
        fg = BWE_ThemeGetColor(BWE_THEME_SELECTION_TEXT);
    } else if (self->control_data.button.is_hovered) {
        bg = BWE_ThemeGetColor(BWE_THEME_WINDOW_BG); // Highlight
        border = BWE_ThemeGetColor(BWE_THEME_WINDOW_BORDER_ACTIVE);
    }

    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, bg);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, border, 2);

    // Centered label text
    int32_t tx = self->screen_bounds.x + 10;
    int32_t ty = self->screen_bounds.y + (self->screen_bounds.height - 12) / 2;
    BWE_DrawText(fb, self->control_data.button.text, tx, ty, fg, 0);

    // If focused, render a dashed inner focus border
    extern uint32_t g_focused_window_id;
    if (self->id == g_focused_window_id) {
        BWE_DrawRect(fb, self->screen_bounds.x + 3, self->screen_bounds.y + 3, self->screen_bounds.width - 6, self->screen_bounds.height - 6, 0x883B82F6, 1);
    }
}

static void bwe_button_event(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    switch (event->type) {
        case BWE_EVENT_MOUSE_DOWN:
            self->control_data.button.is_pressed = true;
            BWE_InvalidateWindow(window_id);
            break;
        case BWE_EVENT_MOUSE_UP:
            if (self->control_data.button.is_pressed) {
                self->control_data.button.is_pressed = false;
                BWE_InvalidateWindow(window_id);
                if (self->control_data.button.on_click) {
                    self->control_data.button.on_click(window_id);
                }
            }
            break;
        case BWE_EVENT_MOUSE_MOVE:
            self->control_data.button.is_hovered = true;
            BWE_InvalidateWindow(window_id);
            break;
        case BWE_EVENT_KEY_DOWN:
            if (event->data.key.key_code == 0x20 || event->data.key.key_code == 0x0D) {
                self->control_data.button.is_pressed = true;
                BWE_InvalidateWindow(window_id);
            }
            break;
        case BWE_EVENT_KEY_UP:
            if (self->control_data.button.is_pressed) {
                self->control_data.button.is_pressed = false;
                BWE_InvalidateWindow(window_id);
                if (self->control_data.button.on_click) {
                    self->control_data.button.on_click(window_id);
                }
            }
            break;
        default:
            break;
    }
}

bwe_error_t BOS_CreateButton(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, void (*on_click)(uint32_t), uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_BUTTON;
        win->control_data.button.bg_color = BWE_ThemeGetColor(BWE_THEME_CONTROL_BG);
        win->control_data.button.text_color = BWE_ThemeGetColor(BWE_THEME_TEXT);
        win->control_data.button.is_pressed = false;
        win->control_data.button.is_hovered = false;
        win->control_data.button.on_click = on_click;

        if (text) {
            uint32_t i = 0;
            for (; text[i] != '\0' && i < 127; i++) {
                win->control_data.button.text[i] = text[i];
            }
            win->control_data.button.text[i] = '\0';
        } else {
            win->control_data.button.text[0] = '\0';
        }

        win->on_render = bwe_button_render;
        win->on_event = bwe_button_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
