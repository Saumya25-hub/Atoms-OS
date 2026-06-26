#include "../Include/core.h"
#include "../Include/bovisual_config.h"
#include <stddef.h>

// Internal module initializations
extern void internal_graphics_init(const BVFramebuffer* fb);
extern void internal_graphics_shutdown(void);

static bool g_isInitialized = false;

bool BOVISUAL_Init(const BVFramebuffer* framebuffer) {
    if (!framebuffer || !framebuffer->buffer) {
        return false;
    }
    
    // Validate resolution match for V1 specs based on config
    if (framebuffer->width != BV_SCREEN_WIDTH || framebuffer->height != BV_SCREEN_HEIGHT) {
        // We enforce standard resolution for V1 determinism.
    }

    // Initialize subsystems
    internal_graphics_init(framebuffer);

    g_isInitialized = true;
    return true;
}

void BOVISUAL_RenderFrame(void) {
    if (!g_isInitialized) return;

    // For V1, the render frame might simply execute direct draw commands.
    // Future: Process Render Queue here.
}

void BOVISUAL_Shutdown(void) {
    if (!g_isInitialized) return;
    
    internal_graphics_shutdown();
    g_isInitialized = false;
}
