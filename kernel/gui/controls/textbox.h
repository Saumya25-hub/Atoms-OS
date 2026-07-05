#ifndef GUI_CONTROLS_TEXTBOX_H
#define GUI_CONTROLS_TEXTBOX_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    char text[256];
    int cursor_pos;
    int max_length;
    bool read_only;
    bool show_caret;
    // For caret blinking timer
    uint64_t last_caret_toggle_tick;
} BOSTextBox;

BOSTextBox* textbox_create();
void textbox_set_text(BOSTextBox* textbox, const char* text);
const char* textbox_get_text(BOSTextBox* textbox);

#endif // GUI_CONTROLS_TEXTBOX_H
