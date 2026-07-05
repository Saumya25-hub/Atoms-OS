#ifndef GUI_CONTROLS_SLIDER_H
#define GUI_CONTROLS_SLIDER_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    int min_val;
    int max_val;
    int value;
    bool dragging;
    void (*on_value_changed)(BOSControl* self, int value);
} BOSSlider;

BOSSlider* slider_create();
void slider_set_range(BOSSlider* slider, int min_val, int max_val);
void slider_set_value(BOSSlider* slider, int value);

#endif // GUI_CONTROLS_SLIDER_H
