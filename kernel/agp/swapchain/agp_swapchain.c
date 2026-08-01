// Engine 4: Swapchain Engine (Triple Buffering)
#include "../include/agp_api.h"
#include "kernel/wm/bwe/include/bwe.h"

int32_t AGP_SwapBuffers(AGPContextID ctx_id) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx || ctx->context_id != ctx_id || !ctx->surface) return -1;

    // Blit surface pixels to BWE Window or frame buffer if window_id is valid
    if (ctx->surface->window_id > 0) {
        BWE_Window* win = BWE_GetWindow(ctx->surface->window_id);
        if (win) {
            BWE_InvalidateWindow(ctx->surface->window_id);
        }
    }
    return 0;
}
