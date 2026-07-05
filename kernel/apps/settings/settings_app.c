#include "settings_app.h"
#include "kernel/gui/window/window.h"

void settings_app_launch(void) {
    struct BOSWindow* win = window_create("ATOMS Settings", 500, 400, 12);
    if (win) {
        window_show(win);
        window_focus(win);
    }
}
