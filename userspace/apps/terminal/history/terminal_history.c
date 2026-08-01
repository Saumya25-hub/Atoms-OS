#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — History Manager Engine
// Arrow-key history with O(1) ring-buffer lookup.
// ============================================================

static char     s_history[TERMINAL_MAX_HISTORY][TERMINAL_MAX_LINE_LEN];
static uint32_t s_history_head  = 0;
static uint32_t s_history_count = 0;

void terminal_history_init(void) {
    s_history_head  = 0;
    s_history_count = 0;
    display_print("[TERMINAL_HISTORY] History Manager Initialized (ring buffer, capacity=256).\n");
}

void terminal_history_push(const char* line) {
    if (!line) return;
    uint32_t i = 0;
    while (line[i] && i < TERMINAL_MAX_LINE_LEN - 1) {
        s_history[s_history_head][i] = line[i]; i++;
    }
    s_history[s_history_head][i] = '\0';
    s_history_head = (s_history_head + 1) % TERMINAL_MAX_HISTORY;
    if (s_history_count < TERMINAL_MAX_HISTORY) s_history_count++;
}

const char* terminal_history_get(uint32_t index) {
    if (index >= s_history_count) return "";
    uint32_t real = (s_history_head + TERMINAL_MAX_HISTORY - 1 - index) % TERMINAL_MAX_HISTORY;
    return s_history[real];
}

uint32_t terminal_history_count(void) {
    return s_history_count;
}
