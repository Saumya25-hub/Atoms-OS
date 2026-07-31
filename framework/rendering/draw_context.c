#include "framework/include/bos_ui_rendering.h"
#include "kernel/core/lib/include/string.h"

void BOS_DrawContext_Init(BOS_DrawContext* ctx, uint32_t* buffer, uint32_t width, uint32_t height, uint32_t pitch) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(BOS_DrawContext));
    ctx->pixel_buffer = buffer;
    ctx->width = width;
    ctx->height = height;
    ctx->pitch = pitch;

    /* Base full-screen clip rectangle */
    ctx->clip_stack[0].x = 0;
    ctx->clip_stack[0].y = 0;
    ctx->clip_stack[0].width = width;
    ctx->clip_stack[0].height = height;
    ctx->clip_depth = 1;
}

void BOS_ClipPush(BOS_DrawContext* ctx, BOS_Rect rect) {
    if (!ctx || ctx->clip_depth >= BOS_MAX_CLIP_STACK_DEPTH) return;

    BOS_Rect parent_clip = ctx->clip_stack[ctx->clip_depth - 1];
    BOS_Rect new_clip;

    /* Intersection of Parent and New Rect */
    int32_t x1 = (rect.x > parent_clip.x) ? rect.x : parent_clip.x;
    int32_t y1 = (rect.y > parent_clip.y) ? rect.y : parent_clip.y;
    int32_t x2 = (rect.x + (int32_t)rect.width < parent_clip.x + (int32_t)parent_clip.width) ? 
                 (rect.x + (int32_t)rect.width) : (parent_clip.x + (int32_t)parent_clip.width);
    int32_t y2 = (rect.y + (int32_t)rect.height < parent_clip.y + (int32_t)parent_clip.height) ? 
                 (rect.y + (int32_t)rect.height) : (parent_clip.y + (int32_t)parent_clip.height);

    new_clip.x = x1;
    new_clip.y = y1;
    new_clip.width = (x2 > x1) ? (uint32_t)(x2 - x1) : 0;
    new_clip.height = (y2 > y1) ? (uint32_t)(y2 - y1) : 0;

    ctx->clip_stack[ctx->clip_depth++] = new_clip;
}

void BOS_ClipPop(BOS_DrawContext* ctx) {
    if (!ctx || ctx->clip_depth <= 1) return;
    ctx->clip_depth--;
}

bool BOS_GetClip(const BOS_DrawContext* ctx, BOS_Rect* out_clip) {
    if (!ctx || !out_clip || ctx->clip_depth == 0) return false;
    *out_clip = ctx->clip_stack[ctx->clip_depth - 1];
    return true;
}

void BOS_DrawFillRect(BOS_DrawContext* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!ctx || !ctx->pixel_buffer) return;

    BOS_Rect clip;
    if (!BOS_GetClip(ctx, &clip)) return;

    int32_t x1 = (x > clip.x) ? x : clip.x;
    int32_t y1 = (y > clip.y) ? y : clip.y;
    int32_t x2 = (x + (int32_t)w < clip.x + (int32_t)clip.width) ? (x + (int32_t)w) : (clip.x + (int32_t)clip.width);
    int32_t y2 = (y + (int32_t)h < clip.y + (int32_t)clip.height) ? (y + (int32_t)h) : (clip.y + (int32_t)clip.height);

    if (x2 <= x1 || y2 <= y1) return;

    uint32_t stride = ctx->pitch / 4;
    for (int32_t cy = y1; cy < y2; cy++) {
        uint32_t* row = &ctx->pixel_buffer[cy * stride + x1];
        for (int32_t cx = x1; cx < x2; cx++) {
            *row++ = color;
        }
    }
}
