#include "../../../wm/bwe/include/bwe.h"

// Expose internal Z-order stack or window pool
extern BWE_Window* BWE_GetWindow(uint32_t window_id);

static void bwe_listview_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Fill white box background
    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, 0xFFFFFFFF);
    
    // Border
    extern uint32_t g_focused_window_id;
    uint32_t border_color = (self->id == g_focused_window_id) ? BWE_ThemeGetColor(BWE_THEME_WINDOW_BORDER_ACTIVE) : BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, border_color, 1);

    uint32_t count = self->control_data.listview.item_count;
    int32_t selected = self->control_data.listview.selected_index;
    int32_t item_height = 20;

    int32_t start_y = self->screen_bounds.y + 2;

    for (uint32_t i = 0; i < count; i++) {
        int32_t iy = start_y + (int32_t)i * item_height;
        if (iy + item_height > self->screen_bounds.y + self->screen_bounds.height) break; // Clip vertically

        if ((int32_t)i == selected) {
            // Selected item background highlight
            BWE_FillRect(fb, self->screen_bounds.x + 2, iy, self->screen_bounds.width - 4, item_height, BWE_ThemeGetColor(BWE_THEME_SELECTION_BG));
            BWE_DrawText(fb, self->control_data.listview.items[i], self->screen_bounds.x + 8, iy + 4, BWE_ThemeGetColor(BWE_THEME_SELECTION_TEXT), 0);
        } else {
            BWE_DrawText(fb, self->control_data.listview.items[i], self->screen_bounds.x + 8, iy + 4, BWE_ThemeGetColor(BWE_THEME_TEXT), 0);
        }
    }
}

static void bwe_listview_event(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    int32_t item_height = 20;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t my = event->data.mouse.y - self->screen_bounds.y;
        int32_t clicked_idx = (my - 2) / item_height;

        if (clicked_idx >= 0 && clicked_idx < (int32_t)self->control_data.listview.item_count) {
            self->control_data.listview.selected_index = clicked_idx;
            BWE_InvalidateWindow(window_id);

            if (self->control_data.listview.on_select) {
                self->control_data.listview.on_select(window_id, clicked_idx);
            }
        }
    } else if (event->type == BWE_EVENT_KEY_DOWN) {
        int32_t sel = self->control_data.listview.selected_index;
        if (event->data.key.key_code == 0x26) { // Up Arrow
            if (sel > 0) {
                self->control_data.listview.selected_index--;
                BWE_InvalidateWindow(window_id);
                if (self->control_data.listview.on_select) {
                    self->control_data.listview.on_select(window_id, sel - 1);
                }
            }
        } else if (event->data.key.key_code == 0x28) { // Down Arrow
            if (sel < (int32_t)self->control_data.listview.item_count - 1) {
                self->control_data.listview.selected_index++;
                BWE_InvalidateWindow(window_id);
                if (self->control_data.listview.on_select) {
                    self->control_data.listview.on_select(window_id, sel + 1);
                }
            }
        }
    }
}

bwe_error_t BOS_CreateListView(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_LISTVIEW;
        win->control_data.listview.item_count = 0;
        win->control_data.listview.selected_index = -1;
        win->control_data.listview.scroll_offset = 0;
        win->control_data.listview.on_select = 0;
        win->control_data.listview.on_double_click = 0;

        win->on_render = bwe_listview_render;
        win->on_event = bwe_listview_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}

bwe_error_t BOS_ListView_AddItem(uint32_t list_id, const char* item) {
    BWE_Window* win = BWE_GetWindow(list_id);
    if (!win || win->type != BWE_TYPE_LISTVIEW) return BWE0001;

    uint32_t idx = win->control_data.listview.item_count;
    if (idx >= 16) return BWE0004; // List size limit

    if (item) {
        uint32_t i = 0;
        for (; item[i] != '\0' && i < 63; i++) {
            win->control_data.listview.items[idx][i] = item[i];
        }
        win->control_data.listview.items[idx][i] = '\0';
    } else {
        win->control_data.listview.items[idx][0] = '\0';
    }

    win->control_data.listview.item_count++;
    BWE_InvalidateWindow(list_id);
    return BWE_SUCCESS;
}
