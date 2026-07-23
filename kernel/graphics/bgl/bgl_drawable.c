#include "bgl_drawable.h"
#include "bgl_context.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/wm/bwe/include/bwe.h"

extern BWE_Window* BWE_GetWindow(uint32_t window_id);

static BGLDrawable s_drawable_pool[BGL_MAX_DRAWABLES];
static uint32_t    s_next_drawable_id = 1;

BGLDrawable* bglCreateDrawableForWindow(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return NULL;

    extern void BWE_Geometry_CalculateClientBounds(BWE_Window* w, BWE_Rect* o);
    BWE_Rect client_rect;
    BWE_Geometry_CalculateClientBounds(win, &client_rect);

    uint32_t w = (uint32_t)client_rect.width;
    uint32_t h = (uint32_t)client_rect.height;
    if (w == 0 || h == 0) return NULL;

    // Find free drawable slot
    BGLDrawable* d = NULL;
    for (size_t i = 0; i < BGL_MAX_DRAWABLES; i++) {
        if (!s_drawable_pool[i].active) {
            d = &s_drawable_pool[i];
            break;
        }
    }

    if (!d) return NULL;

    size_t color_buf_size   = (size_t)w * (size_t)h * sizeof(uint32_t);
    size_t depth_buf_size   = (size_t)w * (size_t)h * sizeof(float);
    size_t stencil_buf_size = (size_t)w * (size_t)h * sizeof(uint8_t);

    uint32_t* color_buf = (uint32_t*)kmalloc_aligned(color_buf_size, 16);
    if (!color_buf) return NULL;

    float* depth_buf = (float*)kmalloc_aligned(depth_buf_size, 16);
    if (!depth_buf) {
        kfree_aligned(color_buf);
        return NULL;
    }

    uint8_t* stencil_buf = (uint8_t*)kmalloc_aligned(stencil_buf_size, 16);
    if (!stencil_buf) {
        kfree_aligned(color_buf);
        kfree_aligned(depth_buf);
        return NULL;
    }

    // Zero-fill color & stencil buffers, initialize depth buffer to 1.0f
    for (size_t i = 0; i < (size_t)(w * h); i++) {
        color_buf[i]   = 0x00000000;
        depth_buf[i]   = 1.0f;
        stencil_buf[i] = 0;
    }

    d->drawable_id    = s_next_drawable_id++;
    d->window_id      = window_id;
    d->owner_pid      = win->owner_pid;
    d->width          = w;
    d->height         = h;
    d->pitch          = w * (uint32_t)sizeof(uint32_t);
    d->pixel_format   = BGL_FORMAT_ARGB8888;
    d->color_buffer   = color_buf;
    d->depth_buffer   = depth_buf;
    d->stencil_buffer = stencil_buf;
    d->generation     = 1;
    d->active         = true;
    d->is_dirty       = true;

    return d;
}

bool bglDestroyDrawable(BGLDrawable* d) {
    if (!d || !d->active) return false;

    // Detach from any context that references this drawable
    bglDetachDrawableFromContexts(d);

    if (d->window_id != 0) {
        BWE_Window* win = BWE_GetWindow(d->window_id);
        if (win && win->control_data.canvas.pixel_buffer == d->color_buffer) {
            win->control_data.canvas.pixel_buffer = NULL;
            win->control_data.canvas.buffer_w = 0;
            win->control_data.canvas.buffer_h = 0;
        }
    }

    if (d->color_buffer) {
        kfree_aligned(d->color_buffer);
        d->color_buffer = NULL;
    }

    if (d->depth_buffer) {
        kfree_aligned(d->depth_buffer);
        d->depth_buffer = NULL;
    }

    if (d->stencil_buffer) {
        kfree_aligned(d->stencil_buffer);
        d->stencil_buffer = NULL;
    }

    d->active     = false;
    d->window_id  = 0;
    d->width      = 0;
    d->height     = 0;
    d->pitch      = 0;
    d->generation = 0;

    return true;
}

bool bglResizeDrawable(BGLDrawable* d, uint32_t new_w, uint32_t new_h) {
    if (!d || !d->active || new_w == 0 || new_h == 0) return false;

    // If dimensions unchanged, no realloc needed
    if (d->width == new_w && d->height == new_h && d->color_buffer != NULL && d->depth_buffer != NULL && d->stencil_buffer != NULL) {
        return true;
    }

    size_t color_buf_size   = (size_t)new_w * (size_t)new_h * sizeof(uint32_t);
    size_t depth_buf_size   = (size_t)new_w * (size_t)new_h * sizeof(float);
    size_t stencil_buf_size = (size_t)new_w * (size_t)new_h * sizeof(uint8_t);

    uint32_t* new_color_buf = (uint32_t*)kmalloc_aligned(color_buf_size, 16);
    if (!new_color_buf) return false;

    float* new_depth_buf = (float*)kmalloc_aligned(depth_buf_size, 16);
    if (!new_depth_buf) {
        kfree_aligned(new_color_buf);
        return false;
    }

    uint8_t* new_stencil_buf = (uint8_t*)kmalloc_aligned(stencil_buf_size, 16);
    if (!new_stencil_buf) {
        kfree_aligned(new_color_buf);
        kfree_aligned(new_depth_buf);
        return false;
    }

    // Zero-fill new color & stencil buffers, init depth buffer to 1.0f
    for (size_t i = 0; i < (size_t)(new_w * new_h); i++) {
        new_color_buf[i]   = 0x00000000;
        new_depth_buf[i]   = 1.0f;
        new_stencil_buf[i] = 0;
    }

    if (d->color_buffer) {
        kfree_aligned(d->color_buffer);
    }
    if (d->depth_buffer) {
        kfree_aligned(d->depth_buffer);
    }
    if (d->stencil_buffer) {
        kfree_aligned(d->stencil_buffer);
    }

    d->color_buffer   = new_color_buf;
    d->depth_buffer   = new_depth_buf;
    d->stencil_buffer = new_stencil_buf;
    d->width          = new_w;
    d->height         = new_h;
    d->pitch          = new_w * (uint32_t)sizeof(uint32_t);
    d->generation++;
    d->is_dirty       = true;

    return true;
}

BGLDrawable* bglGetDrawable(uint32_t drawable_id) {
    if (drawable_id == 0) return NULL;
    for (size_t i = 0; i < BGL_MAX_DRAWABLES; i++) {
        if (s_drawable_pool[i].active && s_drawable_pool[i].drawable_id == drawable_id) {
            return &s_drawable_pool[i];
        }
    }
    return NULL;
}
