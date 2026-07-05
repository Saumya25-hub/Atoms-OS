#ifndef GUI_CONTROLS_LABEL_H
#define GUI_CONTROLS_LABEL_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    char text[128];
    // Future: alignment (Left, Center, Right)
} BOSLabel;

BOSLabel* label_create(const char* text);
void label_set_text(BOSLabel* label, const char* text);

#endif // GUI_CONTROLS_LABEL_H
