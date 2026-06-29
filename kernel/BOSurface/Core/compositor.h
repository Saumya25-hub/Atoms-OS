#ifndef BOSURFACE_COMPOSITOR_H
#define BOSURFACE_COMPOSITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/BOSurface/Core/surface.h"

// Maximum render commands per frame (very generous limit for simple UI)
#define MAX_RENDER_COMMANDS 1024

typedef enum {
    CMD_FILL_RECT,
    CMD_DRAW_RECT,
    CMD_DRAW_BITMAP
} RenderCommandType;

typedef struct {
    RenderCommandType type;
    uint32_t surface_id;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint32_t color;
    void* data; // E.g., for CMD_DRAW_BITMAP
} RenderCommand;

// Damage tracking configuration
#define BOF_MAX_DIRTY_RECTS 8

// Initialize compositor state
void BOCompositor_Init(void);

// Begin a new frame (resets command queue)
void BOCompositor_BeginFrame(void);

// Push a render command to the queue
bool BOCompositor_PushCommand(RenderCommand cmd);

// Execute all queued render commands sequentially (Single Writer phase), clipped to the specified rect
void BOCompositor_RenderFrame(BWE_Rect* clip_rect);

// Add a damage rectangle
void BOCompositor_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height);

// Check if there is any damage to render
bool BOCompositor_HasDamage(void);

// Retrieve damage rects
uint32_t BOCompositor_GetDamageCount(void);
BWE_Rect* BOCompositor_GetDamageRect(uint32_t index);

// Reset damage tracker
void BOCompositor_ClearDamage(void);

#endif // BOSURFACE_COMPOSITOR_H
