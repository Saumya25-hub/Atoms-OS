#include "../include/bosll_api.h"
#include "kernel/drivers/display/display.h"

static bool g_bosll_initialized = false;

BOS_STATUS BosInitialize(void) {
    if (g_bosll_initialized) return BOS_SUCCESS;
    g_bosll_initialized = true;
    display_print("[BOSLL] BOSLL.sll BOS Low-Level Native Runtime V1.0 Initialized\n");
    return BOS_SUCCESS;
}

BOS_STATUS BosShutdown(void) {
    if (!g_bosll_initialized) return BOS_SUCCESS;
    g_bosll_initialized = false;
    display_print("[BOSLL] BOSLL.sll Native Runtime Shutdown Cleanly\n");
    return BOS_SUCCESS;
}
