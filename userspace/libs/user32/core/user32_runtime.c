#include "../include/user32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_user32_initialized = false;

int32_t USER32_Init(void) {
    if (g_user32_initialized) return 0;
    g_user32_initialized = true;
    display_print("[USER32] USER32.sll Ring 3 Runtime V1.0 Initialized\n");
    return 0;
}

int32_t USER32_Shutdown(void) {
    if (!g_user32_initialized) return 0;
    g_user32_initialized = false;
    display_print("[USER32] USER32.sll Runtime Shutdown Cleanly\n");
    return 0;
}
