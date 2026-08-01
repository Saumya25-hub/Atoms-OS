// Engine 6: Window Binding Engine
#include "../include/agp_api.h"
#include "kernel/wm/bwe/include/bwe.h"

int32_t AGP_BindWindow(AGPContextID ctx_id, uint32_t win_id) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx || ctx->context_id != ctx_id || !ctx->surface) return -1;

    ctx->surface->window_id = win_id;
    ctx->surface->is_onscreen = (win_id != 0);
    return 0;
}
