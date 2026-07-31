#include "bgl.h"
#include "kernel/core/scheduler/include/task.h"
#include "kernel/wm/bwe/include/bwe.h"

extern Task* current_task;
extern bwe_error_t BOS_SurfacePresent(uint32_t window_id, const uint32_t* pixels, uint32_t w, uint32_t h);
extern BVFramebuffer* vbe_get_framebuffer(void);

static BGLError s_last_error = BGL_SUCCESS;

BGLError bglGetLastError(void) {
    BGLError err = s_last_error;
    s_last_error = BGL_SUCCESS;
    return err;
}

void bglSetLastError(BGLError err) {
    s_last_error = err;
}

bool bglMakeCurrent(BGLContext* ctx, BGLDrawable* drawable) {
    if (!current_task) {
        bglSetLastError(BGL_ERR_INVALID_CONTEXT);
        return false;
    }

    // Release current context if passing NULL
    if (!ctx && !drawable) {
        bglReleaseCurrent();
        return true;
    }

    if (!ctx || !ctx->active || ctx->state == BGL_STATE_DESTROYED) {
        bglSetLastError(BGL_ERR_INVALID_CONTEXT);
        return false;
    }

    if (!drawable || !drawable->active) {
        bglSetLastError(BGL_ERR_INVALID_DRAWABLE);
        return false;
    }

    // Process ownership validation
    if (ctx->owner_pid != 0 && current_task->owner_pid != 0 && ctx->owner_pid != current_task->owner_pid) {
        bglSetLastError(BGL_ERR_PERMISSION_DENIED);
        return false;
    }

    // Prevent double binding to another task simultaneously
    if (ctx->state == BGL_STATE_CURRENT && ctx->owner_task_id != current_task->id) {
        bglSetLastError(BGL_ERR_ALREADY_CURRENT);
        return false;
    }

    // Unbind previous context from task if present
    if (current_task->current_bgl_context && current_task->current_bgl_context != ctx) {
        BGLContext* prev = (BGLContext*)current_task->current_bgl_context;
        prev->state = BGL_STATE_DETACHED;
    }

    ctx->bound_drawable     = drawable;
    ctx->owner_task_id       = current_task->id;
    ctx->state               = BGL_STATE_CURRENT;
    ctx->drawable_generation = drawable->generation;

    current_task->current_bgl_context = ctx;
    bglSetLastError(BGL_SUCCESS);

    return true;
}

void bglReleaseCurrent(void) {
    if (!current_task || !current_task->current_bgl_context) return;

    BGLContext* ctx = (BGLContext*)current_task->current_bgl_context;
    if (ctx && ctx->active) {
        ctx->state = BGL_STATE_DETACHED;
    }

    current_task->current_bgl_context = NULL;
    bglSetLastError(BGL_SUCCESS);
}

BGLContext* bglGetCurrentContext(void) {
    if (!current_task) return NULL;
    return (BGLContext*)current_task->current_bgl_context;
}

bool bglSwapBuffers(BGLContext* ctx) {
    if (!ctx || !ctx->active) {
        bglSetLastError(BGL_ERR_INVALID_CONTEXT);
        return false;
    }

    if (ctx->state != BGL_STATE_CURRENT || bglGetCurrentContext() != ctx) {
        bglSetLastError(BGL_ERR_NOT_CURRENT);
        return false;
    }

    BGLDrawable* d = ctx->bound_drawable;
    if (!d || !d->active || !d->color_buffer) {
        bglSetLastError(BGL_ERR_INVALID_DRAWABLE);
        return false;
    }

    // Submit BGL drawable color buffer to BWE window canvas
    bwe_error_t err = BOS_SurfacePresent(d->window_id, d->color_buffer, d->width, d->height);
    if (err != BWE_SUCCESS) {
        bglSetLastError(BGL_ERR_SURFACE_NOT_FOUND);
        return false;
    }

    // Trigger desktop composition to pass dirty frame through Compositor and AGDTE
    const BVFramebuffer* hw_fb = vbe_get_framebuffer();
    if (hw_fb) {
        BWE_ComposeFrame(hw_fb);
    }

    ctx->swap_count++;
    if (ctx->swap_count == 1 || (ctx->swap_count % 300) == 0) {
        extern void display_print(const char*);
        extern void display_print_dec(uint32_t);
        display_print("[BGL_PROOF] Frame ");
        display_print_dec(ctx->swap_count);
        display_print(" swapped via BOS OpenGL Context!\n");
    }
    d->is_dirty = false;
    bglSetLastError(BGL_SUCCESS);

    return true;
}
