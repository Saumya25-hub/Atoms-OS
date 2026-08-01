#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

void ExplorerPinTaskbar(HANDLE hwnd) {
    (void)hwnd;
    display_print("[EXPLORER_TASKBAR] Application pinned to Taskbar.\n");
}

void ExplorerUnpinTaskbar(HANDLE hwnd) {
    (void)hwnd;
    display_print("[EXPLORER_TASKBAR] Application unpinned from Taskbar.\n");
}
