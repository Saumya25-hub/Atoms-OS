#include "../include/bwe_render_context.h"
#include "kernel/core/lib/include/string.h"

extern uint32_t BOVISUAL_Graphics_GetWidth(void);
extern uint32_t BOVISUAL_Graphics_GetHeight(void);
extern bool BWE_GetClip(BWE_Rect* out_rect);

bool BWE_RenderContext_CheckClip(int32_t x, int32_t y) {
    BWE_Rect clip;
    if (BWE_GetClip(&clip)) {
        if (x < clip.x || x >= clip.x + clip.width ||
            y < clip.y || y >= clip.y + clip.height) {
            return false;
        }
    } else {
        int32_t sw = (int32_t)BOVISUAL_Graphics_GetWidth();
        int32_t sh = (int32_t)BOVISUAL_Graphics_GetHeight();
        if (x < 0 || x >= sw || y < 0 || y >= sh) {
            return false;
        }
    }
    return true;
}

void BWE_RenderContext_PutPixel(const BVFramebuffer* fb, int32_t x, int32_t y, uint32_t color) {
    if (!fb || !fb->buffer) return;
    
    if (BWE_RenderContext_CheckClip(x, y)) {
        // Fast alpha blend logic since BWE standard UI elements rely on it
        uint8_t a = (color >> 24) & 0xFF;
        if (a == 0) return;
        
        uint32_t offset = y * (fb->pitch / 4) + x;
        if (a == 255) {
            fb->buffer[offset] = color;
        } else {
            uint32_t bg = fb->buffer[offset];
            uint8_t br = (bg >> 16) & 0xFF;
            uint8_t bg_g = (bg >> 8) & 0xFF;
            uint8_t bb = bg & 0xFF;
            
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            
            uint8_t inv_a = 255 - a;
            uint8_t out_r = (r * a + br * inv_a) / 255;
            uint8_t out_g = (g * a + bg_g * inv_a) / 255;
            uint8_t out_b = (b * a + bb * inv_a) / 255;
            
            fb->buffer[offset] = (0xFF << 24) | (out_r << 16) | (out_g << 8) | out_b;
        }
    }
}
