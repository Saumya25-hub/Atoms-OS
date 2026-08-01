#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Console Host Engine
// Owns the console window, rendering, and raw input capture.
// Delegates rendering to USER32.sll / COMCTL32.sll / GDI32.sll
// ============================================================

void terminal_console_init(void) {
    display_print("[TERMINAL_CONSOLE] Console Host Engine Initialized.\n");
}

void terminal_console_render_prompt(void) {
    display_print(TERMINAL_PROMPT);
}

void terminal_console_write(const char* text) {
    if (text) display_print(text);
}

void terminal_console_newline(void) {
    display_print("\n");
}
