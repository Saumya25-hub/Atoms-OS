#include "../../../wm/bwe/include/bwe.h"

static void bwe_textbox_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Fill background
    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, self->control_data.textbox.bg_color);
    
    // Check if focused
    extern uint32_t g_focused_window_id;
    uint32_t border_color = (self->id == g_focused_window_id) ? BWE_ThemeGetColor(BWE_THEME_WINDOW_BORDER_ACTIVE) : BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, border_color, 1);

    // Draw text contents
    int32_t tx = self->screen_bounds.x + 8;
    int32_t ty = self->screen_bounds.y + (self->screen_bounds.height - 12) / 2;

    if (self->control_data.textbox.text[0] != '\0') {
        BWE_DrawText(fb, self->control_data.textbox.text, tx, ty, self->control_data.textbox.text_color, 0);
    } else {
        // Draw placeholder text in dark gray
        BWE_DrawText(fb, self->control_data.textbox.placeholder, tx, ty, 0xFF888888, 0);
    }

    // Render active cursor if focused
    if (self->id == g_focused_window_id) {
        int32_t cursor_x = tx + (int32_t)self->control_data.textbox.cursor_pos * 8;
        if (cursor_x < self->screen_bounds.x + self->screen_bounds.width - 10) {
            BWE_FillRect(fb, cursor_x, ty - 2, 2, 16, BWE_ThemeGetColor(BWE_THEME_TEXT));
        }
    }
}

static void bwe_textbox_event(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t kc = event->data.key.key_code;
        uint32_t ch = (uint8_t)event->data.key.character;
        uint32_t code = (ch >= 32 && ch <= 126) ? ch : kc;
        uint32_t len = 0;
        for (; self->control_data.textbox.text[len] != '\0'; len++);

        if (code == 0x08) { // Backspace
            if (self->control_data.textbox.cursor_pos > 0) {
                uint32_t pos = self->control_data.textbox.cursor_pos;
                for (uint32_t i = pos - 1; i < len; i++) {
                    self->control_data.textbox.text[i] = self->control_data.textbox.text[i + 1];
                }
                self->control_data.textbox.cursor_pos--;
                BWE_InvalidateWindow(window_id);
            }
        } else if (code == 0x2E) { // Delete
            if (self->control_data.textbox.cursor_pos < len) {
                uint32_t pos = self->control_data.textbox.cursor_pos;
                for (uint32_t i = pos; i < len; i++) {
                    self->control_data.textbox.text[i] = self->control_data.textbox.text[i + 1];
                }
                BWE_InvalidateWindow(window_id);
            }
        } else if (code == 0x25) { // Left Arrow
            if (self->control_data.textbox.cursor_pos > 0) {
                self->control_data.textbox.cursor_pos--;
                BWE_InvalidateWindow(window_id);
            }
        } else if (code == 0x27) { // Right Arrow
            if (self->control_data.textbox.cursor_pos < len) {
                self->control_data.textbox.cursor_pos++;
                BWE_InvalidateWindow(window_id);
            }
        } else if (code >= 32 && code <= 126 && len < 127) {
            // Typing characters
            uint32_t pos = self->control_data.textbox.cursor_pos;
            for (uint32_t i = len; i > pos; i--) {
                self->control_data.textbox.text[i] = self->control_data.textbox.text[i - 1];
            }
            self->control_data.textbox.text[pos] = (char)code;
            self->control_data.textbox.text[len + 1] = '\0';
            self->control_data.textbox.cursor_pos++;
            BWE_InvalidateWindow(window_id);
        }
    }
}

bwe_error_t BOS_CreateTextbox(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* placeholder, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_TEXTBOX;
        win->control_data.textbox.bg_color = 0xFFFFFFFF; // Always white
        win->control_data.textbox.text_color = 0xFF000000;
        win->control_data.textbox.cursor_pos = 0;
        win->control_data.textbox.text[0] = '\0';

        if (placeholder) {
            uint32_t i = 0;
            for (; placeholder[i] != '\0' && i < 127; i++) {
                win->control_data.textbox.placeholder[i] = placeholder[i];
            }
            win->control_data.textbox.placeholder[i] = '\0';
        } else {
            win->control_data.textbox.placeholder[0] = '\0';
        }

        win->on_render = bwe_textbox_render;
        win->on_event = bwe_textbox_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}
