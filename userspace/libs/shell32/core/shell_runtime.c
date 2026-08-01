#include "../include/shell32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_shell32_initialized = false;

int32_t ShellInitialize(void) {
    if (g_shell32_initialized) return 0;
    g_shell32_initialized = true;
    display_print("[SHELL32] SHELL32.sll Shell Runtime V1.0 Initialized\n");
    return 0;
}

int32_t ShellShutdown(void) {
    if (!g_shell32_initialized) return 0;
    g_shell32_initialized = false;
    display_print("[SHELL32] SHELL32.sll Shell Runtime Shutdown Cleanly\n");
    return 0;
}

BOOL ShellRefresh(void) {
    return true;
}

BOOL ShellRestartExplorer(void) {
    display_print("[SHELL32] Explorer Subsystem Restarted\n");
    return true;
}
