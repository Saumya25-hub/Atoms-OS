#include "compositor_damage.h"

static BOCompositorRect g_damage_rects[BOCOMPOSITOR_MAX_DAMAGE];
static uint32_t g_damage_count = 0;

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

void BOCompositorDamage_Init(void) {
    g_damage_count = 0;
}

void BOCompositorDamage_AddRect(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) return;

    int32_t x1 = x;
    int32_t y1 = y;
    int32_t x2 = x + width;
    int32_t y2 = y + height;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > (int32_t)g_kernel_screen_width) x2 = (int32_t)g_kernel_screen_width;
    if (y2 > (int32_t)g_kernel_screen_height) y2 = (int32_t)g_kernel_screen_height;

    if (x1 >= x2 || y1 >= y2) return;

    BOCompositorRect rect = {x1, y1, x2 - x1, y2 - y1};

    // Try to merge with any overlapping or adjacent rectangle
    for (uint32_t i = 0; i < g_damage_count; i++) {
        BOCompositorRect* existing = &g_damage_rects[i];
        if (rect.x <= existing->x + existing->width &&
            rect.x + rect.width >= existing->x &&
            rect.y <= existing->y + existing->height &&
            rect.y + rect.height >= existing->y) {
            
            int32_t nx1 = (rect.x < existing->x) ? rect.x : existing->x;
            int32_t ny1 = (rect.y < existing->y) ? rect.y : existing->y;
            int32_t nx2 = (rect.x + rect.width > existing->x + existing->width) ? (rect.x + rect.width) : (existing->x + existing->width);
            int32_t ny2 = (rect.y + rect.height > existing->y + existing->height) ? (rect.y + rect.height) : (existing->y + existing->height);
            
            existing->x = nx1;
            existing->y = ny1;
            existing->width = nx2 - nx1;
            existing->height = ny2 - ny1;
            return;
        }
    }

    if (g_damage_count >= BOCOMPOSITOR_MAX_DAMAGE) {
        // Fallback: merge into slot 0 bounding box
        BOCompositorRect* root = &g_damage_rects[0];
        int32_t nx1 = (rect.x < root->x) ? rect.x : root->x;
        int32_t ny1 = (rect.y < root->y) ? rect.y : root->y;
        int32_t nx2 = (rect.x + rect.width > root->x + root->width) ? (rect.x + rect.width) : (root->x + root->width);
        int32_t ny2 = (rect.y + rect.height > root->y + root->height) ? (rect.y + rect.height) : (root->y + root->height);
        root->x = nx1;
        root->y = ny1;
        root->width = nx2 - nx1;
        root->height = ny2 - ny1;
        return;
    }

    g_damage_rects[g_damage_count++] = rect;
}

void BOCompositorDamage_Clear(void) {
    g_damage_count = 0;
}

bool BOCompositorDamage_HasDamage(void) {
    return g_damage_count > 0;
}

uint32_t BOCompositorDamage_GetCount(void) {
    return g_damage_count;
}

const BOCompositorRect* BOCompositorDamage_GetRect(uint32_t index) {
    if (index >= g_damage_count) return NULL;
    return &g_damage_rects[index];
}
