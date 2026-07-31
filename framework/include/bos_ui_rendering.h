#ifndef BOS_UI_RENDERING_H
#define BOS_UI_RENDERING_H

#include "bos_ui_core.h"

#define BOS_MAX_CLIP_STACK_DEPTH 32U

struct BOS_DrawContext {
    uint32_t* pixel_buffer;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    
    BOS_Rect  clip_stack[BOS_MAX_CLIP_STACK_DEPTH];
    uint32_t  clip_depth;
};

void BOS_DrawContext_Init(BOS_DrawContext* ctx, uint32_t* buffer, uint32_t width, uint32_t height, uint32_t pitch);
void BOS_ClipPush(BOS_DrawContext* ctx, BOS_Rect rect);
void BOS_ClipPop(BOS_DrawContext* ctx);
bool BOS_GetClip(const BOS_DrawContext* ctx, BOS_Rect* out_clip);

void BOS_DrawFillRect(BOS_DrawContext* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color);
void BOS_DrawBorderRect(BOS_DrawContext* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t thickness, uint32_t radius);
void BOS_DrawText(BOS_DrawContext* ctx, const char* text, int32_t x, int32_t y, uint32_t color);

#endif /* BOS_UI_RENDERING_H */
