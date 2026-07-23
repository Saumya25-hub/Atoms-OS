#ifndef BGL_CONTEXT_H
#define BGL_CONTEXT_H

#include "bgl_drawable.h"

typedef struct BGLContext {
    uint32_t        context_id;
    uint32_t        owner_pid;
    uint32_t        owner_task_id;
    BGLDrawable*    bound_drawable;
    BGLContextState state;
    uint32_t        clear_color;
    uint32_t        drawable_generation;
    uint64_t        swap_count;
    uint64_t        clear_count;
    void*           private_data;   // Reserved for Phase 2 OpenGL state machine
    bool            active;
} BGLContext;

BGLContext* bglCreateContext(BGLDrawable* drawable);
bool        bglDestroyContext(BGLContext* ctx);
void        bglDiagnosticClear(BGLContext* ctx, uint32_t color);
BGLContext* bglGetContext(uint32_t context_id);
void        bglDetachDrawableFromContexts(BGLDrawable* drawable);

#endif // BGL_CONTEXT_H
