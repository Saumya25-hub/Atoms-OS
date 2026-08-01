// Engine 7: GPU Command Queue & Immediate Mode Renderer
#include "../include/agp_api.h"

void AGP_Clear(uint32_t color, float depth) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx || !ctx->surface || !ctx->surface->pixels) return;

    ctx->clear_color = color;
    ctx->clear_depth = depth;

    uint32_t count = ctx->surface->width * ctx->surface->height;
    uint32_t* dst = ctx->surface->pixels;
    for (uint32_t i = 0; i < count; i++) {
        dst[i] = color;
    }
}

void AGP_Begin(AGPPrimitiveType prim) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx) return;

    ctx->imm_primitive = prim;
    ctx->imm_count = 0;
    ctx->in_begin_end = true;
}

void AGP_Vertex3f(float x, float y, float z) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx || !ctx->in_begin_end) return;

    if (ctx->imm_count < 1024) {
        AGPVertex* v = &ctx->imm_vertices[ctx->imm_count++];
        v->x = x; v->y = y; v->z = z;
        v->r = 1.0f; v->g = 1.0f; v->b = 1.0f; v->a = 1.0f;
        v->u = 0.0f; v->v = 0.0f;
    }
}

void AGP_Color4f(float r, float g, float b, float a) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx) return;
    (void)r; (void)g; (void)b; (void)a;
}

void AGP_TexCoord2f(float u, float v) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx) return;
    (void)u; (void)v;
}

void AGP_End(void) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (!ctx || !ctx->in_begin_end) return;

    ctx->in_begin_end = false;
    // Draw primitives into active surface
    if (ctx->surface && ctx->surface->pixels && ctx->imm_count > 0) {
        // Draw rasterization software fallback for primitives into surface
        uint32_t color = 0xFFFFFFFF;
        for (uint32_t i = 0; i < ctx->imm_count; i++) {
            int32_t px = (int32_t)((ctx->imm_vertices[i].x + 1.0f) * 0.5f * ctx->surface->width);
            int32_t py = (int32_t)((1.0f - ctx->imm_vertices[i].y) * 0.5f * ctx->surface->height);
            if (px >= 0 && px < (int32_t)ctx->surface->width && py >= 0 && py < (int32_t)ctx->surface->height) {
                ctx->surface->pixels[py * ctx->surface->width + px] = color;
            }
        }
    }
}

void AGP_DrawArrays(AGPPrimitiveType prim, uint32_t first, uint32_t count) {
    (void)prim; (void)first; (void)count;
}
