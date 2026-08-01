#include "../include/kernel32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_kernel32_initialized = false;

int32_t KERNEL32_Init(void) {
    if (g_kernel32_initialized) return 0;
    g_kernel32_initialized = true;
    display_print("[KERNEL32] KERNEL32.sll Production System Runtime V1.0 Initialized\n");
    return 0;
}

int32_t KERNEL32_Shutdown(void) {
    if (!g_kernel32_initialized) return 0;
    g_kernel32_initialized = false;
    display_print("[KERNEL32] KERNEL32.sll System Runtime Shutdown Cleanly\n");
    return 0;
}
