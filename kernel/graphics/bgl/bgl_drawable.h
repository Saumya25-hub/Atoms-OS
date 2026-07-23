#ifndef BGL_DRAWABLE_H
#define BGL_DRAWABLE_H

#include "bgl_types.h"

typedef struct BGLDrawable {
    uint32_t        drawable_id;
    uint32_t        window_id;
    uint32_t        owner_pid;
    uint32_t        width;
    uint32_t        height;
    uint32_t        pitch;          // Stride in bytes (width * 4)
    BGLPixelFormat  pixel_format;
    uint32_t*       color_buffer;   // 16-byte aligned offscreen color buffer
    float*          depth_buffer;   // 16-byte aligned offscreen depth buffer
    uint8_t*        stencil_buffer; // 16-byte aligned offscreen stencil buffer (8-bit)
    uint32_t        generation;     // Incremented on resize/reallocation
    bool            active;
    bool            is_dirty;
} BGLDrawable;

BGLDrawable* bglCreateDrawableForWindow(uint32_t window_id);
bool         bglDestroyDrawable(BGLDrawable* drawable);
bool         bglResizeDrawable(BGLDrawable* drawable, uint32_t new_w, uint32_t new_h);
BGLDrawable* bglGetDrawable(uint32_t drawable_id);

#endif // BGL_DRAWABLE_H
