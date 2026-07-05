#include "files_app.h"
#include "kernel/gui/window/window.h"

void files_app_launch(void) {
    struct BOSWindow* win = window_create("ATOMS Files", 700, 500, 13);
    if (win) {
        window_show(win);
        window_focus(win);
    }
}
