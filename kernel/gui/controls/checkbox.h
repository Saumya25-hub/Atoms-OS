#ifndef GUI_CONTROLS_CHECKBOX_H
#define GUI_CONTROLS_CHECKBOX_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    char text[64];
    bool checked;
    void (*on_toggle)(BOSControl* self, bool checked);
} BOSCheckBox;

BOSCheckBox* checkbox_create(const char* text);
void checkbox_set_checked(BOSCheckBox* checkbox, bool checked);
bool checkbox_is_checked(BOSCheckBox* checkbox);

#endif // GUI_CONTROLS_CHECKBOX_H
