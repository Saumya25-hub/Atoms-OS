#ifndef GUI_CONTROLS_RADIOBUTTON_H
#define GUI_CONTROLS_RADIOBUTTON_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    char text[64];
    bool selected;
    int group_id; // Radio buttons with same group ID are mutually exclusive
    void (*on_select)(BOSControl* self);
} BOSRadioButton;

BOSRadioButton* radiobutton_create(const char* text, int group_id);
void radiobutton_set_selected(BOSRadioButton* radiobutton, bool selected);

#endif // GUI_CONTROLS_RADIOBUTTON_H
