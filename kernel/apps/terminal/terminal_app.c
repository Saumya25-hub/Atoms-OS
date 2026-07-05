#include "terminal_app.h"
#include "kernel/gui/window/window.h"

void terminal_app_launch(void) {
    struct BOSWindow* win = window_create("ATOMS Terminal", 640, 480, 10); // Dummy PID 10
    if (win) {
        window_show(win);
        window_focus(win);
    }
}
