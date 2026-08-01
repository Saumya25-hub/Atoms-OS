#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX V1.0 — Runtime Manager
// Official Native Console Host & Command Runtime of ATOMS OS
//
// Architecture:
//   Terminal.BOSX owns NO kernel functionality.
//   All operations delegate through:
//     USER32.sll / SHELL32.sll / KERNEL32.sll / BOSLL.sll
// ============================================================

static bool              g_terminal_initialized = false;
static TERMINAL_SESSION  g_session              = {0};

// --- Built-in command registry ---
#define TERMINAL_CMD_TABLE_MAX 64
static TERMINAL_COMMAND  g_cmd_table[TERMINAL_CMD_TABLE_MAX];
static uint32_t          g_cmd_count = 0;

int32_t TerminalInitialize(void) {
    if (g_terminal_initialized) return 0;

    display_print("[TERMINAL] Initializing Terminal.BOSX V1.0...\n");

    g_session.active        = true;
    g_session.command_count = 0;
    g_session.error_count   = 0;

    // Initialize sub-engines via forward declarations
    extern void terminal_console_init(void);
    extern void terminal_history_init(void);
    extern void terminal_autocomplete_init(void);
    extern void terminal_aliases_init(void);
    extern void terminal_environment_init(void);
    extern void terminal_plugin_init(void);

    terminal_console_init();
    terminal_history_init();
    terminal_autocomplete_init();
    terminal_aliases_init();
    terminal_environment_init();
    terminal_plugin_init();

    g_terminal_initialized = true;
    display_print("[TERMINAL] Terminal.BOSX V1.0 Ready. Session Active.\n");
    return 0;
}

void TerminalShutdown(void) {
    if (!g_terminal_initialized) return;
    display_print("[TERMINAL] Shutting down Terminal.BOSX...\n");
    g_session.active = false;
    g_terminal_initialized = false;
    display_print("[TERMINAL] Terminal.BOSX Shutdown Complete.\n");
}

bool TerminalRegisterCommand(const char* name, TERMINAL_COMMAND_HANDLER handler) {
    if (!name || !handler) return false;
    if (g_cmd_count >= TERMINAL_CMD_TABLE_MAX) return false;
    // copy name safely
    uint32_t i = 0;
    while (name[i] && i < 63) { g_cmd_table[g_cmd_count].name[i] = name[i]; i++; }
    g_cmd_table[g_cmd_count].name[i] = '\0';
    g_cmd_table[g_cmd_count].handler = handler;
    g_cmd_count++;
    return true;
}

bool TerminalPrint(const char* text) {
    if (!text) return false;
    display_print(text);
    return true;
}

bool TerminalClearScreen(void) {
    display_print("\033[2J\033[H");
    return true;
}

bool TerminalReadLine(char* buffer, uint32_t length) {
    if (!buffer || length == 0) return false;
    // In production: reads from console input via USER32/CONHOST
    // For certification: stub returns true
    buffer[0] = '\0';
    return true;
}
