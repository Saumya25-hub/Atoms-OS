// Engine 11: Pipeline State Manager
#include "../include/agp_api.h"

void AGP_EnableCapability(uint32_t cap, bool enable) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx) return;

    if (cap == 0x0B71) ctx->depth_test = enable;    // GL_DEPTH_TEST
    else if (cap == 0x0BE2) ctx->blend_enabled = enable; // GL_BLEND
    else if (cap == 0x0B44) ctx->cull_face = enable;    // GL_CULL_FACE
}
