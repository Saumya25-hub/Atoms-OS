#include "../include/advapi32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_advapi32_initialized = false;

int32_t AdvApiInitialize(void) {
    if (g_advapi32_initialized) return 1;
    g_advapi32_initialized = true;
    display_print("[ADVAPI32] ADVAPI32.sll Security, Registry & Service Control Runtime V1.0 Initialized\n");
    return 1;
}

int32_t AdvApiShutdown(void) {
    if (!g_advapi32_initialized) return 1;
    g_advapi32_initialized = false;
    display_print("[ADVAPI32] ADVAPI32.sll Runtime Shutdown Cleanly\n");
    return 1;
}
