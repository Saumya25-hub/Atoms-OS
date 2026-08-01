#include "../include/bar_api.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static bool g_bar_initialized = false;
static uint32_t g_active_runtimes = 0;

int32_t BAR_Init(void) {
    if (g_bar_initialized) return 0;
    
    g_active_runtimes = 0;
    g_bar_initialized = true;
    
    display_print("[BAR_CORE] BOS Application Runtime (BAR V1.0) Initialized\n");
    return 0;
}

int32_t BAR_Shutdown(void) {
    if (!g_bar_initialized) return 0;
    g_active_runtimes = 0;
    g_bar_initialized = false;
    display_print("[BAR_CORE] BAR Subsystem Shutdown Cleanly\n");
    return 0;
}

BARHandle BAR_CreateRuntime(const char* name) {
    if (!g_bar_initialized || !name) return 0;
    g_active_runtimes++;
    return (BARHandle)(g_active_runtimes + 100);
}

int32_t BAR_DestroyRuntime(BARHandle runtime) {
    if (!g_bar_initialized || runtime == 0) return -1;
    if (g_active_runtimes > 0) g_active_runtimes--;
    return 0;
}
