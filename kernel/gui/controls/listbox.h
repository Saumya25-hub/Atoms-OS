#ifndef GUI_CONTROLS_LISTBOX_H
#define GUI_CONTROLS_LISTBOX_H

#include "kernel/gui/controls/control.h"

#define LISTBOX_MAX_ITEMS 64
#define LISTBOX_ITEM_LEN 64

typedef struct {
    BOSControl base;
    char items[LISTBOX_MAX_ITEMS][LISTBOX_ITEM_LEN];
    int item_count;
    int selected_index;
    int scroll_offset;
    void (*on_selection_changed)(BOSControl* self, int selected_index);
} BOSListBox;

BOSListBox* listbox_create();
void listbox_add_item(BOSListBox* listbox, const char* item);
void listbox_set_selected(BOSListBox* listbox, int index);

#endif // GUI_CONTROLS_LISTBOX_H
