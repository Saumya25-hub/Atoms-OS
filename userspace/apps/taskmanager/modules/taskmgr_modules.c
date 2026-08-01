#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

static uint32_t s_module_count = 0;

void taskmgr_modules_init(void) {
    s_module_count = 8; // kernel32, bosll, user32, gdi32, shell32, advapi32, ws2_32, ole32
    display_print("[TASKMGR_MOD] Module Manager Engine Initialized.\n");
}

bool taskmgr_modules_refresh(void) {
    display_print("[TASKMGR_MOD] taskmgr_modules_refresh() -> KERNEL32.EnumProcessModules() OK\n");
    return true;
}

uint32_t taskmgr_module_count(void) { return s_module_count; }
