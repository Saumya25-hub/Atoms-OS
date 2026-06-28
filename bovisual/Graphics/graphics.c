#include "../Include/graphics.h"
#include <stddef.h>

static BVFramebuffer g_active_fb = {0};
static bool g_graphics_ready = false;

// Phase 5: Damage Tracking
static int32_t damage_x1 = 99999;
static int32_t damage_y1 = 99999;
static int32_t damage_x2 = -1;
static int32_t damage_y2 = -1;

// Global Hardware-style Clipping
static BVRect g_clip_rect = {0, 0, 0, 0};
static bool g_clip_enabled = false;

void BOVISUAL_Graphics_SetClipRect(BVRect clip) {
    g_clip_rect = clip;
    g_clip_enabled = true;
}

void BOVISUAL_Graphics_ClearClipRect(void) {
    g_clip_enabled = false;
}

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
    
    if (g_clip_enabled) {
        if (x < g_clip_rect.x || x >= g_clip_rect.x + g_clip_rect.width ||
            y < g_clip_rect.y || y >= g_clip_rect.y + g_clip_rect.height) {
            return; // Out of clip rect
        }
    }

    uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + (y * g_active_fb.pitch);
    ((BOVISUAL_Color*)row_ptr)[x] = color;
}

BOVISUAL_Color BOVISUAL_Graphics_ReadPixel(int32_t x, int32_t y) {
    if (!g_graphics_ready || !g_active_fb.buffer) return 0;

    if (x < 0 || x >= (int32_t)g_active_fb.width || y < 0 || y >= (int32_t)g_active_fb.height) {
        return 0; // Out of bounds returns 0 (Transparent/Black)
    }

    uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + (y * g_active_fb.pitch);
    return ((BOVISUAL_Color*)row_ptr)[x];
}

void BOVISUAL_Graphics_Clear(BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer) return;

    if (g_clip_enabled) {
        BOVISUAL_Graphics_Fill(g_clip_rect.x, g_clip_rect.y, g_clip_rect.width, g_clip_rect.height, color);
        return;
    }

    for (int32_t row = 0; row < (int32_t)g_active_fb.height; row++) {
        uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + (row * g_active_fb.pitch);
        BOVISUAL_Color* pixel_ptr = (BOVISUAL_Color*)row_ptr;
        for (int32_t col = 0; col < (int32_t)g_active_fb.width; col++) {
            pixel_ptr[col] = color;
        }
    }
}

void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color) {
    if (!g_graphics_ready || !g_active_fb.buffer || width <= 0 || height <= 0) return;

    int32_t cx1 = x;
    int32_t cy1 = y;
    int32_t cx2 = x + width;
    int32_t cy2 = y + height;
    
    // Absolute clipping against screen bounds
    if (cx1 < 0) cx1 = 0;
    if (cy1 < 0) cy1 = 0;
    if (cx2 > (int32_t)g_active_fb.width) cx2 = (int32_t)g_active_fb.width;
    if (cy2 > (int32_t)g_active_fb.height) cy2 = (int32_t)g_active_fb.height;

    // Global Hardware Clipping
    if (g_clip_enabled) {
        if (cx1 < g_clip_rect.x) cx1 = g_clip_rect.x;
        if (cy1 < g_clip_rect.y) cy1 = g_clip_rect.y;
        if (cx2 > g_clip_rect.x + g_clip_rect.width) cx2 = g_clip_rect.x + g_clip_rect.width;
        if (cy2 > g_clip_rect.y + g_clip_rect.height) cy2 = g_clip_rect.y + g_clip_rect.height;
    }

    if (cx1 >= cx2 || cy1 >= cy2) return;

    width = cx2 - cx1;
    height = cy2 - cy1;
    x = cx1;
    y = cy1;

    // Debug integrity check (Log only)
    extern bool BOS_DEBUG_MODE;
    extern void display_print(const char* str);
    if (BOS_DEBUG_MODE && (x < 0 || y < 0 || x + width > (int32_t)g_active_fb.width || y + height > (int32_t)g_active_fb.height)) {
        display_print("[BWE_ERROR] BOVISUAL_Graphics_Fill out of bounds write attempt!\n");
        return;
    }

    for (int32_t row = 0; row < height; row++) {
        uint8_t* row_ptr = (uint8_t*)g_active_fb.buffer + ((y + row) * g_active_fb.pitch);
        BOVISUAL_Color* pixel_ptr = (BOVISUAL_Color*)row_ptr + x;
        for (int32_t col = 0; col < width; col++) {
            pixel_ptr[col] = color;
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
    
    // Fast 64-bit copy ONLY IF stride matches!
    if (w == (int32_t)g_active_fb.width && h == (int32_t)g_active_fb.height && g_active_fb.pitch == hw_fb->pitch) {
        uint32_t total_bytes = g_active_fb.height * g_active_fb.pitch;
        uint64_t* src = (uint64_t*)g_active_fb.buffer;
        uint64_t* dst = (uint64_t*)hw_fb->buffer;
        uint32_t count = total_bytes / 8;
        for (uint32_t i = 0; i < count; i++) {
            dst[i] = src[i];
        }
    } else {
        // Safe row-by-row copy with precise stride arithmetic (avoids stride mismatch corruption)
        for (int32_t row = damage_y1; row <= damage_y2; row++) {
            uint8_t* src_row = (uint8_t*)g_active_fb.buffer + (row * g_active_fb.pitch);
            uint8_t* dst_row = (uint8_t*)hw_fb->buffer + (row * hw_fb->pitch);
            
            uint32_t* src = (uint32_t*)src_row + damage_x1;
            uint32_t* dst = (uint32_t*)dst_row + damage_x1;
            
            for (int32_t col = 0; col < w; col++) {
                dst[col] = src[col];
            }
        }
    }
    
    BOVISUAL_Graphics_ResetDamage();
}

void BOVISUAL_Graphics_SwapRect(const BVFramebuffer* hw_fb, BVRect rect) {
    if (!g_graphics_ready || !g_active_fb.buffer || !hw_fb || !hw_fb->buffer || rect.width <= 0 || rect.height <= 0) return;
    
    int32_t cx1 = rect.x;
    int32_t cy1 = rect.y;
    int32_t cx2 = rect.x + rect.width;
    int32_t cy2 = rect.y + rect.height;
    
    // Clip damage rect to screen boundaries safely
    if (cx1 < 0) cx1 = 0;
    if (cy1 < 0) cy1 = 0;
    if (cx2 > (int32_t)g_active_fb.width) cx2 = (int32_t)g_active_fb.width;
    if (cy2 > (int32_t)g_active_fb.height) cy2 = (int32_t)g_active_fb.height;
    
    if (cx1 >= cx2 || cy1 >= cy2) return;
    
    rect.width = cx2 - cx1;
    rect.height = cy2 - cy1;
    rect.x = cx1;
    rect.y = cy1;

    // Safety check! Ensure hw_fb bounds match
    if (rect.x + rect.width > (int32_t)hw_fb->width) rect.width = hw_fb->width - rect.x;
    if (rect.y + rect.height > (int32_t)hw_fb->height) rect.height = hw_fb->height - rect.y;

    if (rect.width <= 0 || rect.height <= 0) return;
    
    // Copy only the dirty rectangle row by row safely using independent pitches
    for (int32_t row = 0; row < rect.height; row++) {
        uint8_t* src_row = (uint8_t*)g_active_fb.buffer + ((rect.y + row) * g_active_fb.pitch);
        uint8_t* dst_row = (uint8_t*)hw_fb->buffer + ((rect.y + row) * hw_fb->pitch);
        
        uint32_t* src = (uint32_t*)src_row + rect.x;
        uint32_t* dst = (uint32_t*)dst_row + rect.x;
        
        for (int32_t col = 0; col < rect.width; col++) {
            dst[col] = src[col];
        }
    }
}
