#ifndef GUI_CONTROLS_PROGRESSBAR_H
#define GUI_CONTROLS_PROGRESSBAR_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    int min_val;
    int max_val;
    int value;
} BOSProgressBar;

BOSProgressBar* progressbar_create();
void progressbar_set_range(BOSProgressBar* pb, int min_val, int max_val);
void progressbar_set_value(BOSProgressBar* pb, int value);

#endif // GUI_CONTROLS_PROGRESSBAR_H
