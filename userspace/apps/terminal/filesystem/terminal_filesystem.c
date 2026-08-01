#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Filesystem Runtime Engine
// Provides VFS path context for cd/pwd/dir commands.
// Delegates all actual I/O to KERNEL32.sll / SHELL32.sll.
// ============================================================

static char s_cwd[512] = "/";

void terminal_filesystem_init(void) {
    s_cwd[0] = '/'; s_cwd[1] = '\0';
    display_print("[TERMINAL_FS] Filesystem Runtime Context Initialized. CWD=/\n");
}

const char* terminal_filesystem_get_cwd(void) { return s_cwd; }

bool terminal_filesystem_set_cwd(const char* path) {
    if (!path) return false;
    uint32_t i = 0;
    while (path[i] && i < 511) { s_cwd[i] = path[i]; i++; }
    s_cwd[i] = '\0';
    return true;
}
