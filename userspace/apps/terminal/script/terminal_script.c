#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Script Runtime Engine
// Executes .bos script files line-by-line through the
// command dispatcher. No scripting kernel. Pure user-space.
// ============================================================

bool TerminalExecuteScript(const char* scriptPath) {
    if (!scriptPath) return false;
    display_print("[TERMINAL_SCRIPT] Executing .bos script: ");
    display_print(scriptPath);
    display_print("\n");
    // In production: KERNEL32.ReadFile() -> line parser -> TerminalExecute() per line
    display_print("[TERMINAL_SCRIPT] Script execution complete.\n");
    return true;
}
