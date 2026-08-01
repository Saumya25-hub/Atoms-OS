#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Alias Manager Engine
// Maps user-defined short names to full command strings.
// ============================================================

static TERMINAL_ALIAS s_aliases[TERMINAL_MAX_ALIASES];
static uint32_t       s_alias_count = 0;

// Simple safe string copy
static void safe_strcpy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void terminal_aliases_init(void) {
    s_alias_count = 0;
    display_print("[TERMINAL_ALIASES] Alias Manager Initialized.\n");
}

bool TerminalRegisterAlias(const char* alias, const char* target) {
    if (!alias || !target) return false;
    if (s_alias_count >= TERMINAL_MAX_ALIASES) return false;
    safe_strcpy(s_aliases[s_alias_count].alias,  alias,  64);
    safe_strcpy(s_aliases[s_alias_count].target, target, 256);
    s_alias_count++;
    return true;
}

const char* terminal_aliases_resolve(const char* name) {
    if (!name) return 0;
    for (uint32_t i = 0; i < s_alias_count; i++) {
        const char* a = s_aliases[i].alias;
        const char* n = name;
        bool eq = true;
        while (*n && *a) { if (*n != *a) { eq = false; break; } n++; a++; }
        if (eq && *n == '\0' && *a == '\0') return s_aliases[i].target;
    }
    return 0;
}
