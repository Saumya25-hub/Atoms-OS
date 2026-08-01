#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

static bool g_explorer_initialized = false;

int32_t ExplorerInitialize(void) {
    if (g_explorer_initialized) return 0;
    display_print("[EXPLORER] Initializing Explorer.exe V1.0 Desktop Shell & Session Host...\n");
    g_explorer_initialized = true;
    display_print("[EXPLORER] Explorer.exe V1.0 Initialized Successfully.\n");
    return 0;
}

void ExplorerShutdown(void) {
    display_print("[EXPLORER] Shutting down Explorer Desktop Shell...\n");
    g_explorer_initialized = false;
}

void ExplorerRestart(void) {
    ExplorerShutdown();
    ExplorerInitialize();
    ExplorerStartSession();
}

void ExplorerRestartShell(void) {
    display_print("[EXPLORER] Restarting Shell Process (Taskbar/Desktop)...\n");
    ExplorerRefreshDesktop();
}
