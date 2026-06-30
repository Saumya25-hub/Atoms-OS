#include "compositor_clip.h"

static BOCompositorRect g_clip_stack[BOCOMPOSITOR_MAX_CLIPS];
static uint32_t g_clip_stack_top = 0;
static BOCompositorRect g_screen_rect = {0, 0, 1920, 1080};

void BOCompositorClip_Init(int32_t screen_width, int32_t screen_height) {
    g_screen_rect = (BOCompositorRect){0, 0, screen_width, screen_height};
    g_clip_stack[0] = g_screen_rect;
    g_clip_stack_top = 1;
}

bool BOCompositorClip_Intersect(const BOCompositorRect* a, const BOCompositorRect* b, BOCompositorRect* out_res) {
    if (!a || !b || !out_res) return false;

    int32_t x1 = (a->x > b->x) ? a->x : b->x;
    int32_t y1 = (a->y > b->y) ? a->y : b->y;

    int32_t ax2 = a->x + a->width;
    int32_t bx2 = b->x + b->width;
    int32_t x2 = (ax2 < bx2) ? ax2 : bx2;

    int32_t ay2 = a->y + a->height;
    int32_t by2 = b->y + b->height;
    int32_t y2 = (ay2 < by2) ? ay2 : by2;

    if (x1 >= x2 || y1 >= y2) {
        out_res->x = 0;
        out_res->y = 0;
        out_res->width = 0;
        out_res->height = 0;
        return false;
    }

    out_res->x = x1;
    out_res->y = y1;
    out_res->width = x2 - x1;
    out_res->height = y2 - y1;
    return true;
}

void BOCompositorClip_PushRect(const BOCompositorRect* rect) {
    if (!rect || g_clip_stack_top >= BOCOMPOSITOR_MAX_CLIPS) return;

    BOCompositorRect current = g_clip_stack[g_clip_stack_top - 1];
    BOCompositorRect intersection;
    if (BOCompositorClip_Intersect(&current, rect, &intersection)) {
        g_clip_stack[g_clip_stack_top++] = intersection;
    } else {
        // Empty intersection
        g_clip_stack[g_clip_stack_top++] = (BOCompositorRect){0, 0, 0, 0};
    }
}

void BOCompositorClip_PopRect(void) {
    if (g_clip_stack_top > 1) {
        g_clip_stack_top--;
    }
}

bool BOCompositorClip_GetCurrent(BOCompositorRect* out_rect) {
    if (!out_rect || g_clip_stack_top == 0) return false;
    *out_rect = g_clip_stack[g_clip_stack_top - 1];
    return (out_rect->width > 0 && out_rect->height > 0);
}
