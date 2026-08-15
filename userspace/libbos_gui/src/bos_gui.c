#include "../include/bos_gui.h"
#include "../include/syscalls_gui.h"
#include "../../libbos/include/bos.h"

void BOS_Run(void) {
    BOS_GUIEvent event;
    while (1) {
        if (sys_gui_poll_event(0, &event)) {
            if (event.type == BOS_GUI_EVENT_CLOSE) {
                break;
            }
        } else {
            bos_yield();
        }
    }
}
