#include "bgl_context.h"
#include "kernel/core/scheduler/include/task.h"

extern Task* current_task;

static BGLContext s_context_pool[BGL_MAX_CONTEXTS];
static uint32_t   s_next_context_id = 1;

#include "kernel/graphics/gl/gl_state.h"

BGLContext* bglCreateContext(BGLDrawable* drawable) {
    if (!drawable || !drawable->active) return NULL;

    BGLContext* ctx = NULL;
    for (size_t i = 0; i < BGL_MAX_CONTEXTS; i++) {
        if (!s_context_pool[i].active) {
            ctx = &s_context_pool[i];
            break;
        }
    }

    if (!ctx) return NULL;

    GLContextState* gl_state = gl_state_create();
    if (!gl_state) return NULL;

    ctx->context_id          = s_next_context_id++;
    ctx->owner_pid           = current_task ? current_task->owner_pid : 0;
    ctx->owner_task_id       = current_task ? current_task->id : 0;
    ctx->bound_drawable     = drawable;
    ctx->state               = BGL_STATE_CREATED;
    ctx->clear_color         = 0xFF000000; // Default opaque black
    ctx->drawable_generation = drawable->generation;
    ctx->swap_count          = 0;
    ctx->clear_count         = 0;
    ctx->private_data        = (void*)gl_state;
    ctx->active              = true;

    return ctx;
}

bool bglDestroyContext(BGLContext* ctx) {
    if (!ctx || !ctx->active) return false;

    // Detach from current task if currently bound to prevent dangling pointer
    if (current_task && current_task->current_bgl_context == ctx) {
        current_task->current_bgl_context = NULL;
    }

    ctx->state = BGL_STATE_DESTROYED;

    if (ctx->private_data) {
        gl_state_destroy((GLContextState*)ctx->private_data);
        ctx->private_data = NULL;
    }

    ctx->active              = false;
    ctx->bound_drawable     = NULL;
    ctx->owner_pid           = 0;
    ctx->owner_task_id       = 0;
    ctx->drawable_generation = 0;

    return true;
}

void bglDiagnosticClear(BGLContext* ctx, uint32_t color) {
    if (!ctx || !ctx->active || ctx->state != BGL_STATE_CURRENT) return;

    BGLDrawable* d = ctx->bound_drawable;
    if (!d || !d->active || !d->color_buffer) return;

    // Auto-sync generation if drawable was resized
    if (ctx->drawable_generation != d->generation) {
        ctx->drawable_generation = d->generation;
    }

    uint32_t w = d->width;
    uint32_t h = d->height;
    uint32_t stride = d->pitch / sizeof(uint32_t); // Exact stride elements per row
    uint32_t* buf = d->color_buffer;

    ctx->clear_color = color;

    for (uint32_t y = 0; y < h; y++) {
        uint32_t row_offset = y * stride;
        for (uint32_t x = 0; x < w; x++) {
            buf[row_offset + x] = color;
        }
    }

    d->is_dirty = true;
    ctx->clear_count++;
}

BGLContext* bglGetContext(uint32_t context_id) {
    if (context_id == 0) return NULL;
    for (size_t i = 0; i < BGL_MAX_CONTEXTS; i++) {
        if (s_context_pool[i].active && s_context_pool[i].context_id == context_id) {
            return &s_context_pool[i];
        }
    }
    return NULL;
}

void bglDetachDrawableFromContexts(BGLDrawable* drawable) {
    if (!drawable) return;
    for (size_t i = 0; i < BGL_MAX_CONTEXTS; i++) {
        if (s_context_pool[i].active && s_context_pool[i].bound_drawable == drawable) {
            s_context_pool[i].bound_drawable = NULL;
        }
    }
}
