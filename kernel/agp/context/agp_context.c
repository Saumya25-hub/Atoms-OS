// Engine 2: Graphics Context Engine
#include "../include/agp_api.h"
#include "kernel/core/lib/include/string.h"

extern void AGP_SetCurrentContextInternal(AGPContext* ctx);

static AGPContext s_context_pool[AGP_MAX_CONTEXTS];
static bool s_context_used[AGP_MAX_CONTEXTS];

AGPContextID AGP_CreateContext(uint32_t win_id, uint32_t w, uint32_t h) {
    for (uint32_t i = 0; i < AGP_MAX_CONTEXTS; i++) {
        if (!s_context_used[i]) {
            s_context_used[i] = true;
            AGPContext* ctx = &s_context_pool[i];
            memset(ctx, 0, sizeof(AGPContext));
            
            ctx->context_id = i + 1;
            ctx->surface = AGP_CreateSurface(w, h, AGP_FORMAT_RGBA8888);
            if (ctx->surface) {
                ctx->surface->window_id = win_id;
                ctx->surface->is_onscreen = (win_id != 0);
            }

            ctx->viewport = (AGPRect){0, 0, (int32_t)w, (int32_t)h};
            ctx->clear_color = 0xFF000000;
            ctx->clear_depth = 1.0f;
            ctx->depth_test = true;
            ctx->ref_count = 1;

            // Identity matrices
            ctx->modelview[0] = 1.0f; ctx->modelview[5] = 1.0f; ctx->modelview[10] = 1.0f; ctx->modelview[15] = 1.0f;
            ctx->projection[0] = 1.0f; ctx->projection[5] = 1.0f; ctx->projection[10] = 1.0f; ctx->projection[15] = 1.0f;

            return ctx->context_id;
        }
    }
    return 0;
}

void AGP_DestroyContext(AGPContextID ctx_id) {
    if (ctx_id == 0 || ctx_id > AGP_MAX_CONTEXTS) return;
    uint32_t idx = ctx_id - 1;
    if (s_context_used[idx]) {
        AGPContext* ctx = &s_context_pool[idx];
        if (ctx->surface) {
            AGP_DestroySurface(ctx->surface);
        }
        if (AGP_GetCurrentContext() == ctx) {
            AGP_SetCurrentContextInternal(NULL);
        }
        s_context_used[idx] = false;
    }
}

int32_t AGP_MakeCurrent(AGPContextID ctx_id) {
    if (ctx_id == 0) {
        AGP_SetCurrentContextInternal(NULL);
        return 0;
    }
    if (ctx_id > AGP_MAX_CONTEXTS) return -1;
    uint32_t idx = ctx_id - 1;
    if (!s_context_used[idx]) return -1;

    AGP_SetCurrentContextInternal(&s_context_pool[idx]);
    return 0;
}
