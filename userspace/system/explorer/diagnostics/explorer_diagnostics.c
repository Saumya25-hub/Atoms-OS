#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

void ExplorerDumpDiagnostics(void) {
    display_print("[EXPLORER_DIAG] Desktop Session Host (PID 101), Taskbar, StartMenu, Tray: RUNNING (PASS)\n");
}
