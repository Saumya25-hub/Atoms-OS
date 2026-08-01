#include "../include/opengl32_api.h"
#include "kernel/drivers/display/display.h"

static bool g_opengl32_initialized = false;

int32_t OpenGLInitialize(void) {
    if (g_opengl32_initialized) return 1;
    g_opengl32_initialized = true;
    display_print("[OPENGL32] OpenGL32.sll Ring 3 Graphics API Runtime V1.0 Initialized\n");
    return 1;
}

int32_t OpenGLShutdown(void) {
    if (!g_opengl32_initialized) return 1;
    g_opengl32_initialized = false;
    display_print("[OPENGL32] OpenGL32.sll Ring 3 Runtime Shutdown Cleanly\n");
    return 1;
}
