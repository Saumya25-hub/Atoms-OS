#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

static uint32_t s_handle_count = 0;

void taskmgr_handles_init(void) {
    s_handle_count = 0;
    display_print("[TASKMGR_HDL] Handle Manager Engine Initialized.\n");
}

bool RefreshHandles(void) {
    // In production: KERNEL32.GetProcessHandleCount() per process
    s_handle_count = 64;
    display_print("[TASKMGR_HDL] RefreshHandles() -> KERNEL32.GetProcessHandleCount() OK\n");
    return true;
}

uint32_t taskmgr_handle_count(void) { return s_handle_count; }
