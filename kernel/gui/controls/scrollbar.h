#ifndef GUI_CONTROLS_SCROLLBAR_H
#define GUI_CONTROLS_SCROLLBAR_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    bool horizontal;
    int min_val;
    int max_val;
    int value;
    int page_size;
    bool dragging;
    void (*on_scroll)(BOSControl* self, int value);
} BOSScrollBar;

BOSScrollBar* scrollbar_create(bool horizontal);

#endif // GUI_CONTROLS_SCROLLBAR_H
