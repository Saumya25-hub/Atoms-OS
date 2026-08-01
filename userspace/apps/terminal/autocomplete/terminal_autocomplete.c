#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Auto Complete Engine
// TAB-key completion against the built-in command table.
// ============================================================

static const char* s_completion_candidates[] = {
    "dir","ls","cd","pwd","mkdir","rmdir","copy","move","del","rename","type",
    "start","tasklist","taskkill","run",
    "cls","echo","ver","hostname","whoami","time","date","exit",
    "set","unset","env","path",
    "ping","ipconfig","netstat",
    "mem","cpu","handles","uptime","diagnostics",
    "help","history","alias","clearhistory","reload"
};
#define COMPLETION_COUNT ((uint32_t)(sizeof(s_completion_candidates)/sizeof(s_completion_candidates[0])))

void terminal_autocomplete_init(void) {
    display_print("[TERMINAL_AUTOCOMPLETE] Auto Complete Engine Initialized.\n");
}

const char* terminal_autocomplete_query(const char* prefix, uint32_t index) {
    if (!prefix) return 0;
    uint32_t match_idx = 0;
    for (uint32_t i = 0; i < COMPLETION_COUNT; i++) {
        const char* c = s_completion_candidates[i];
        const char* p = prefix;
        bool matches = true;
        while (*p) { if (*p != *c) { matches = false; break; } p++; c++; }
        if (matches) {
            if (match_idx == index) return s_completion_candidates[i];
            match_idx++;
        }
    }
    return 0;
}
