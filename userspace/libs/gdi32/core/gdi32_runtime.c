#include "../include/gdi32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_gdi32_initialized = false;

int32_t GDI32_Init(void) {
    if (g_gdi32_initialized) return 0;
    g_gdi32_initialized = true;
    display_print("[GDI32] GDI32.sll Ring 3 Graphics Runtime V1.0 Initialized\n");
    return 0;
}

int32_t GDI32_Shutdown(void) {
    if (!g_gdi32_initialized) return 0;
    g_gdi32_initialized = false;
    display_print("[GDI32] GDI32.sll Runtime Shutdown Cleanly\n");
    return 0;
}
