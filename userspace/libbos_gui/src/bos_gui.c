#include "../include/bos_gui.h"
#include "../include/syscalls_gui.h"
#include "../../libbos/include/bos.h"

void BOS_Run(void) {
    BOS_GUIEvent event;
    while (1) {
        if (sys_gui_get_event(&event)) {
            if (event.type == BOS_GUI_EVENT_CLICK) {
                if (!BOS_Internal_ProcessWidgetEvent(&event)) {
                    if (event.user_callback) {
                        void (*cb)(void) = (void (*)(void))event.user_callback;
                        cb();
                    }
                }
            } else if (event.type == BOS_GUI_EVENT_CLOSE) {
                break;
            }
        } else {
            bos_yield();
        }
    }
}
