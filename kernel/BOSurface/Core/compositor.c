#include "kernel/BOSurface/Core/compositor.h"
#include "kernel/display/display.h"
#include "kernel/lib/include/string.h"
#include "bovisual/Include/graphics.h"

static RenderCommand render_queue[MAX_RENDER_COMMANDS];
static uint32_t render_queue_count = 0;

static BWE_Rect damage_rects[BOF_MAX_DIRTY_RECTS];
static uint32_t damage_count = 0;

uint32_t g_compositor_current_surface_id = 0;

extern void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);

void BOCompositor_Init(void) {
    render_queue_count = 0;
    damage_count = 0;
}

void BOCompositor_BeginFrame(void) {
    render_queue_count = 0;
}

bool BOCompositor_PushCommand(RenderCommand cmd) {
    if (render_queue_count >= MAX_RENDER_COMMANDS) {
        return false;
    }
    if (cmd.surface_id == 0) {
        cmd.surface_id = g_compositor_current_surface_id;
    }
    render_queue[render_queue_count++] = cmd;
    return true;
}

// Global Hardware Clipping from BOVISUAL
extern void BOVISUAL_Graphics_SetClipRect(BVRect clip);
extern void BOVISUAL_Graphics_ClearClipRect(void);

void BOCompositor_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) return;
    
    BWE_Rect rect = {x, y, width, height};

    // Hard Validation: Ensure inside screen bounds
    extern uint32_t g_kernel_screen_width;
    extern uint32_t g_kernel_screen_height;
    
    int32_t cx1 = rect.x;
    int32_t cy1 = rect.y;
    int32_t cx2 = rect.x + rect.width;
    int32_t cy2 = rect.y + rect.height;
    
    if (cx1 < 0) cx1 = 0;
    if (cy1 < 0) cy1 = 0;
    if (cx2 > (int32_t)g_kernel_screen_width) cx2 = (int32_t)g_kernel_screen_width;
    if (cy2 > (int32_t)g_kernel_screen_height) cy2 = (int32_t)g_kernel_screen_height;
    
    if (cx1 >= cx2 || cy1 >= cy2) return;
    
    rect.x = cx1;
    rect.y = cy1;
    rect.width = cx2 - cx1;
    rect.height = cy2 - cy1;

    // Merge overlapping rects
    for (uint32_t i = 0; i < damage_count; i++) {
        BWE_Rect* existing = &damage_rects[i];
        
        // Check intersection
        if (rect.x < existing->x + existing->width &&
            rect.x + rect.width > existing->x &&
            rect.y < existing->y + existing->height &&
            rect.y + rect.height > existing->y) {
            
            // Merge rect into existing bounding box
            int32_t nx1 = (rect.x < existing->x) ? rect.x : existing->x;
            int32_t ny1 = (rect.y < existing->y) ? rect.y : existing->y;
            int32_t nx2 = (rect.x + rect.width > existing->x + existing->width) ? (rect.x + rect.width) : (existing->x + existing->width);
            int32_t ny2 = (rect.y + rect.height > existing->y + existing->height) ? (rect.y + rect.height) : (existing->y + existing->height);
            
            existing->x = nx1;
            existing->y = ny1;
            existing->width = nx2 - nx1;
            existing->height = ny2 - ny1;
            
            return; // Merged
        }
    }

    if (damage_count >= BOF_MAX_DIRTY_RECTS) {
        // Safe Mode: Fallback to full screen if too many fragmented rects
        damage_count = 1;
        damage_rects[0].x = 0;
        damage_rects[0].y = 0;
        damage_rects[0].width = g_kernel_screen_width;
        damage_rects[0].height = g_kernel_screen_height;
        return;
    }

    damage_rects[damage_count++] = rect;
}

bool BOCompositor_HasDamage(void) {
    return damage_count > 0;
}

uint32_t BOCompositor_GetDamageCount(void) {
    return damage_count;
}

BWE_Rect* BOCompositor_GetDamageRect(uint32_t index) {
    if (index >= damage_count) return NULL;
    return &damage_rects[index];
}

// Resets damage count for the next frame
void BOCompositor_ClearDamage(void) {
    damage_count = 0;
}

void BOCompositor_PushCommand_FromGraphics(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color) {
    RenderCommand cmd = {
        .type = CMD_FILL_RECT,
        .surface_id = g_compositor_current_surface_id,
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .color = color,
        .data = 0
    };
    BOCompositor_PushCommand(cmd);
}

extern void BOVISUAL_Graphics_Fill_Internal(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);

void BOCompositor_RenderFrame(BWE_Rect* clip_rect) {
    // Single Writer Phase: Process the render queue sequentially
    
    if (clip_rect) {
        BVRect clip = {clip_rect->x, clip_rect->y, clip_rect->width, clip_rect->height};
        BOVISUAL_Graphics_SetClipRect(clip);
    }
    
    for (uint32_t i = 0; i < render_queue_count; i++) {
        RenderCommand* cmd = &render_queue[i];
        
        switch (cmd->type) {
            case CMD_FILL_RECT:
                BOVISUAL_Graphics_Fill_Internal(cmd->x, cmd->y, cmd->width, cmd->height, cmd->color);
                break;
                
            case CMD_DRAW_BITMAP: {
                // If it's a cursor bitmap, we draw it pixel by pixel or with a dedicated fn
                break;
            }
                
            default:
                break;
        }
    }
    
    if (clip_rect) {
        BOVISUAL_Graphics_ClearClipRect();
    }
}
