#ifndef GUI_CONTROLS_BUTTON_H
#define GUI_CONTROLS_BUTTON_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    char text[64];
    // Function pointer for click event
    void (*on_click)(BOSControl* self);
} BOSButton;

BOSButton* button_create(const char* text);
void button_set_text(BOSButton* button, const char* text);
void button_set_on_click(BOSButton* button, void (*on_click)(BOSControl* self));

#endif // GUI_CONTROLS_BUTTON_H
