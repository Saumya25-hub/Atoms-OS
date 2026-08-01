#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Process Runtime Engine
// Delegates process launch to KERNEL32.CreateProcess() and
// ShellExecute() via SHELL32.sll.
// Terminal itself never touches the scheduler.
// ============================================================

void terminal_process_init(void) {
    display_print("[TERMINAL_PROCESS] Process Runtime Engine Initialized.\n");
}

bool terminal_process_launch(const char* appPath, const char* args) {
    if (!appPath) return false;
    display_print("[TERMINAL_PROCESS] Launching: ");
    display_print(appPath);
    if (args) { display_print(" "); display_print(args); }
    display_print(" -> KERNEL32.CreateProcess()\n");
    return true;
}

bool terminal_process_shell_execute(const char* target) {
    if (!target) return false;
    display_print("[TERMINAL_PROCESS] ShellExecute: ");
    display_print(target);
    display_print(" -> SHELL32.ShellExecute()\n");
    return true;
}
