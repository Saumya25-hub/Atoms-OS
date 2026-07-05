#include "kernel/gui/controls/textbox.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <string.h>

static void textbox_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSTextBox* txt = (BOSTextBox*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    
    BVRect rect = {ax, ay, self->width, self->height};
    
    // Draw background
    BOVISUAL_Color bg_color = {20, 20, 20, 255};
    if (self->focused) {
        bg_color = (BOVISUAL_Color){30, 30, 30, 255};
    }
    painter_fill_rect(surface, &rect, bg_color, clip);
    
    // Draw border
    BOVISUAL_Color border_color = self->focused ? (BOVISUAL_Color){100, 200, 255, 255} : (BOVISUAL_Color){100, 100, 100, 255};
    painter_draw_rect(surface, &rect, border_color, clip);
    
    // Draw Text (stub)
    // BOVISUAL_Color text_color = {255, 255, 255, 255};
    // int tx = ax + 4;
    // int ty = ay + (self->height - 16) / 2;
    // painter_draw_text(surface, tx, ty, txt->text, text_color, clip);
    
    // Draw Caret
    if (self->focused && txt->show_caret) {
        int caret_x = ax + 4 + (txt->cursor_pos * 8); // Assuming 8px char width
        int caret_y1 = ay + 4;
        int caret_y2 = ay + self->height - 4;
        BOVISUAL_Color caret_color = {255, 255, 255, 255};
        painter_draw_line(surface, caret_x, caret_y1, caret_x, caret_y2, caret_color, clip);
    }
}

static void textbox_handle_event(BOSControl* self, const GUIEvent* event) {
    BOSTextBox* txt = (BOSTextBox*)self;
    if (!self->enabled) return;
    
    switch (event->type) {
        case GUI_EVENT_MOUSE_DOWN:
            self->focused = true;
            control_invalidate(self);
            break;
            
        case GUI_EVENT_KEY_DOWN:
            if (!self->focused || txt->read_only) break;
            
            uint32_t key = event->data.key.key_code;
            int len = strlen(txt->text);
            
            // Simple backspace
            if (key == 0x08) { // Assuming 0x08 is backspace
                if (txt->cursor_pos > 0) {
                    // Shift left
                    for (int i = txt->cursor_pos - 1; i < len; i++) {
                        txt->text[i] = txt->text[i + 1];
                    }
                    txt->cursor_pos--;
                    control_invalidate(self);
                }
            } 
            // Simple character append
            else if (key >= 0x20 && key <= 0x7E) {
                if (len < txt->max_length - 1) {
                    // Shift right to make space for insert
                    for (int i = len; i >= txt->cursor_pos; i--) {
                        txt->text[i + 1] = txt->text[i];
                    }
                    txt->text[txt->cursor_pos] = (char)key;
                    txt->cursor_pos++;
                    control_invalidate(self);
                }
            }
            break;
            
        default:
            break;
    }
}

static void textbox_destroy(BOSControl* self) {
    kfree(self);
}

BOSTextBox* textbox_create() {
    BOSTextBox* txt = (BOSTextBox*)kmalloc(sizeof(BOSTextBox));
    if (!txt) return NULL;
    
    control_init(&txt->base);
    txt->base.paint = textbox_paint;
    txt->base.handle_event = textbox_handle_event;
    txt->base.destroy = textbox_destroy;
    
    txt->base.width = 150;
    txt->base.height = 24;
    
    txt->text[0] = '\0';
    txt->cursor_pos = 0;
    txt->max_length = sizeof(txt->text) - 1;
    txt->read_only = false;
    txt->show_caret = true;
    txt->last_caret_toggle_tick = 0;
    
    return txt;
}

void textbox_set_text(BOSTextBox* textbox, const char* text) {
    if (!textbox) return;
    strncpy(textbox->text, text ? text : "", textbox->max_length);
    textbox->text[textbox->max_length] = '\0';
    textbox->cursor_pos = strlen(textbox->text);
    control_invalidate(&textbox->base);
}

const char* textbox_get_text(BOSTextBox* textbox) {
    if (!textbox) return NULL;
    return textbox->text;
}
