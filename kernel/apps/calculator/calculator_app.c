#include "calculator_app.h"
#include "kernel/gui/window/window.h"

void calculator_app_launch(void) {
    struct BOSWindow* win = window_create("ATOMS Calculator", 250, 350, 14);
    if (win) {
        window_show(win);
        window_focus(win);
    }
}
