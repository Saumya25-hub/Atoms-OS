#ifndef GUI_CONTROLS_PANEL_H
#define GUI_CONTROLS_PANEL_H

#include "kernel/gui/controls/control.h"

typedef struct {
    BOSControl base;
    bool transparent; // If true, doesn't draw a background
} BOSPanel;

BOSPanel* panel_create();
void panel_set_transparent(BOSPanel* panel, bool transparent);

#endif // GUI_CONTROLS_PANEL_H
