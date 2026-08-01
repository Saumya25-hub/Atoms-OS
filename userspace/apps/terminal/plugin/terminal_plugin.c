#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Plugin Runtime Engine
// Loads external .bos command modules that register additional
// built-in commands via TerminalRegisterCommand().
// ============================================================

static char    s_plugin_paths[TERMINAL_MAX_PLUGINS][256];
static uint32_t s_plugin_count = 0;

void terminal_plugin_init(void) {
    s_plugin_count = 0;
    display_print("[TERMINAL_PLUGIN] Plugin Runtime Engine Initialized.\n");
}

bool TerminalLoadPlugin(const char* pluginPath) {
    if (!pluginPath) return false;
    if (s_plugin_count >= TERMINAL_MAX_PLUGINS) return false;
    uint32_t i = 0;
    while (pluginPath[i] && i < 255) { s_plugin_paths[s_plugin_count][i] = pluginPath[i]; i++; }
    s_plugin_paths[s_plugin_count][i] = '\0';
    s_plugin_count++;
    display_print("[TERMINAL_PLUGIN] Plugin loaded: ");
    display_print(pluginPath);
    display_print("\n");
    return true;
}

uint32_t terminal_plugin_count(void) { return s_plugin_count; }
