#include "../Include/graphics.h"
#include <stddef.h>

static BVFramebuffer g_active_fb = {0};
static bool g_graphics_ready = false;

// Phase 5: Damage Tracking
static int32_t damage_x1 = 99999;
static int32_t damage_y1 = 99999;
static int32_t damage_x2 = -1;
static int32_t damage_y2 = -1;

// Called by Core during initialization
void internal_graphics_init(const BVFramebuffer* fb) {
    if (fb) {
        g_active_fb = *fb;
        g_graphics_ready = true;
    }
}

// Called by Core during shutdown
void internal_graphics_shutdown(void) {
    g_active_fb.buffer = NULL;
    g_graphics_ready = false;
}

void BOVISUAL_Graphics_PutPixel(int32_t x, int32_t y, BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer) return;

    if (x < 0 || x >= (int32_t)g_active_fb.width || y < 0 || y >= (int32_t)g_active_fb.height) {
        return; // Clip to screen bounds
    }

    uint32_t offset = (y * (g_active_fb.pitch / sizeof(BOVISUAL_Color))) + x;
    g_active_fb.buffer[offset] = color;
}

BOVISUAL_Color BOVISUAL_Graphics_ReadPixel(int32_t x, int32_t y) {
    if (!g_graphics_ready || !g_active_fb.buffer) return 0;

    if (x < 0 || x >= (int32_t)g_active_fb.width || y < 0 || y >= (int32_t)g_active_fb.height) {
        return 0; // Out of bounds returns 0 (Transparent/Black)
    }

    uint32_t offset = (y * (g_active_fb.pitch / sizeof(BOVISUAL_Color))) + x;
    return g_active_fb.buffer[offset];
}

void BOVISUAL_Graphics_Clear(BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer) return;

    // Direct loop, can be replaced by SIMD/DMA internally in the future without changing API
    uint32_t total_pixels = g_active_fb.width * g_active_fb.height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        g_active_fb.buffer[i] = color;
    }
}

void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer) return;

    // Absolute clipping against screen bounds
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    
    if (x + width > (int32_t)g_active_fb.width) { width = g_active_fb.width - x; }
    if (y + height > (int32_t)g_active_fb.height) { height = g_active_fb.height - y; }

    if (width <= 0 || height <= 0) return;

    uint32_t pitch_pixels = g_active_fb.pitch / sizeof(BOVISUAL_Color);
    
    for (int32_t row = 0; row < height; row++) {
        uint32_t offset = ((y + row) * pitch_pixels) + x;
        for (int32_t col = 0; col < width; col++) {
            g_active_fb.buffer[offset + col] = color;
        }
    }
}

void BOVISUAL_Graphics_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (x < damage_x1) damage_x1 = x;
    if (y < damage_y1) damage_y1 = y;
    if (x + width - 1 > damage_x2) damage_x2 = x + width - 1;
    if (y + height - 1 > damage_y2) damage_y2 = y + height - 1;
}

void BOVISUAL_Graphics_ResetDamage(void) {
    damage_x1 = 99999;
    damage_y1 = 99999;
    damage_x2 = -1;
    damage_y2 = -1;
}

void BOVISUAL_Graphics_SwapBuffers(const BVFramebuffer* hw_fb) {
    if (!g_graphics_ready || !g_active_fb.buffer || !hw_fb || !hw_fb->buffer) return;
    
    // If no damage, skip swap entirely
    if (damage_x1 > damage_x2 || damage_y1 > damage_y2) return;
    
    // Clip damage rect to screen boundaries
    if (damage_x1 < 0) damage_x1 = 0;
    if (damage_y1 < 0) damage_y1 = 0;
    if (damage_x2 >= (int32_t)g_active_fb.width) damage_x2 = g_active_fb.width - 1;
    if (damage_y2 >= (int32_t)g_active_fb.height) damage_y2 = g_active_fb.height - 1;
    
    int32_t w = damage_x2 - damage_x1 + 1;
    int32_t h = damage_y2 - damage_y1 + 1;
    if (w <= 0 || h <= 0) return;
    
    // If damage spans the entire screen, use fast 64-bit copy for the whole buffer
    if (w == (int32_t)g_active_fb.width && h == (int32_t)g_active_fb.height) {
        uint32_t total_bytes = g_active_fb.height * g_active_fb.pitch;
        uint64_t* src = (uint64_t*)g_active_fb.buffer;
        uint64_t* dst = (uint64_t*)hw_fb->buffer;
        uint32_t count = total_bytes / 8;
        for (uint32_t i = 0; i < count; i++) {
            dst[i] = src[i];
        }
    } else {
        // Copy only the dirty rectangle row by row
        uint32_t pitch_pixels = g_active_fb.pitch / sizeof(BOVISUAL_Color);
        for (int32_t row = damage_y1; row <= damage_y2; row++) {
            uint32_t offset = (row * pitch_pixels) + damage_x1;
            uint32_t* src = (uint32_t*)&g_active_fb.buffer[offset];
            uint32_t* dst = (uint32_t*)&hw_fb->buffer[offset];
            for (int32_t col = 0; col < w; col++) {
                dst[col] = src[col];
            }
        }
    }
    
    BOVISUAL_Graphics_ResetDamage();
}
