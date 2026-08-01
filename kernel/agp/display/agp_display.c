// Engine 5: Display Manager
#include "../include/agp_api.h"

void AGP_SetViewport(int32_t x, int32_t y, uint32_t w, uint32_t h) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx) return;

    ctx->viewport.x = x;
    ctx->viewport.y = y;
    ctx->viewport.width = (int32_t)w;
    ctx->viewport.height = (int32_t)h;
}
