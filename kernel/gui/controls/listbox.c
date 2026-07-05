#include "kernel/gui/controls/listbox.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/render/painter.h"
#include <string.h>

static void listbox_paint(BOSControl* self, struct BOSSurface* surface, const BVRect* clip) {
    BOSListBox* lb = (BOSListBox*)self;
    if (!self->visible) return;
    
    int ax, ay;
    control_get_absolute_position(self, &ax, &ay);
    BVRect rect = {ax, ay, self->width, self->height};
    
    BOVISUAL_Color bg_color = {255, 255, 255, 255};
    painter_fill_rect(surface, &rect, bg_color, clip);
    
    BOVISUAL_Color border_color = {100, 100, 100, 255};
    painter_draw_rect(surface, &rect, border_color, clip);
    
    // Draw items (stub)
    // int item_height = 16;
    // for (int i = 0; i < lb->item_count; i++) {
    //     int draw_y = ay + (i * item_height);
    //     if (draw_y + item_height > ay + self->height) break;
    //     if (i == lb->selected_index) {
    //         BVRect sel_rect = {ax, draw_y, self->width, item_height};
    //         BOVISUAL_Color sel_bg = {0, 120, 215, 255};
    //         painter_fill_rect(surface, &sel_rect, sel_bg, clip);
    //     }
    //     // painter_draw_text(surface, ax + 2, draw_y + 2, lb->items[i], text_color, clip);
    // }
}

static void listbox_handle_event(BOSControl* self, const GUIEvent* event) {
    BOSListBox* lb = (BOSListBox*)self;
    if (!self->enabled) return;
    
    if (event->type == GUI_EVENT_MOUSE_DOWN) {
        self->focused = true;
        // Simple selection logic stub
        // int item_height = 16;
        // int local_y = event->data.mouse.y - self->y; // Need absolute y here though
        // int clicked_index = local_y / item_height;
        // if (clicked_index < lb->item_count) {
        //     listbox_set_selected(lb, clicked_index);
        // }
    }
}

static void listbox_destroy(BOSControl* self) {
    kfree(self);
}

BOSListBox* listbox_create() {
    BOSListBox* lb = (BOSListBox*)kmalloc(sizeof(BOSListBox));
    if (!lb) return NULL;
    
    control_init(&lb->base);
    lb->base.paint = listbox_paint;
    lb->base.handle_event = listbox_handle_event;
    lb->base.destroy = listbox_destroy;
    
    lb->base.width = 120;
    lb->base.height = 100;
    
    lb->item_count = 0;
    lb->selected_index = -1;
    lb->scroll_offset = 0;
    lb->on_selection_changed = NULL;
    
    return lb;
}

void listbox_add_item(BOSListBox* listbox, const char* item) {
    if (!listbox || !item) return;
    if (listbox->item_count < LISTBOX_MAX_ITEMS) {
        strncpy(listbox->items[listbox->item_count], item, LISTBOX_ITEM_LEN - 1);
        listbox->item_count++;
        control_invalidate(&listbox->base);
    }
}

void listbox_set_selected(BOSListBox* listbox, int index) {
    if (!listbox) return;
    if (index >= -1 && index < listbox->item_count) {
        listbox->selected_index = index;
        control_invalidate(&listbox->base);
        if (listbox->on_selection_changed) {
            listbox->on_selection_changed(&listbox->base, index);
        }
    }
}
