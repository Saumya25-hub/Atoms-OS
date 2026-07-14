#include "pointer_bounds.h"
#include "kernel/drivers/display/display.h"

static PointerDisplayBounds g_displays[POINTER_MAX_DISPLAYS];
static uint32_t g_display_count = 0;

void pointer_bounds_init(uint32_t primary_width, uint32_t primary_height) {
    for (int i = 0; i < POINTER_MAX_DISPLAYS; i++) {
        g_displays[i].active = false;
    }
    
    g_displays[0].origin_x = 0;
    g_displays[0].origin_y = 0;
    g_displays[0].width  = (primary_width > 0) ? primary_width : 1280;
    g_displays[0].height = (primary_height > 0) ? primary_height : 720;
    g_displays[0].dpi_scale = 100;
    g_displays[0].active = true;
    g_display_count = 1;

    display_print("[POINTER BOUNDS] Multi-Monitor Bounding Box Registry Initialized.\n");
}

bool pointer_bounds_add_display(int32_t origin_x, int32_t origin_y, uint32_t width, uint32_t height, uint32_t dpi_scale) {
    if (g_display_count >= POINTER_MAX_DISPLAYS || width == 0 || height == 0) {
        return false;
    }

    g_displays[g_display_count].origin_x = origin_x;
    g_displays[g_display_count].origin_y = origin_y;
    g_displays[g_display_count].width  = width;
    g_displays[g_display_count].height = height;
    g_displays[g_display_count].dpi_scale = (dpi_scale > 0) ? dpi_scale : 100;
    g_displays[g_display_count].active = true;
    g_display_count++;
    return true;
}

void pointer_bounds_clamp(int32_t* inout_x, int32_t* inout_y) {
    if (!inout_x || !inout_y || g_display_count == 0) {
        return;
    }

    int32_t x = *inout_x;
    int32_t y = *inout_y;

    // Check if point lies within any active display rectangle
    for (uint32_t i = 0; i < g_display_count; i++) {
        if (!g_displays[i].active) continue;
        int32_t min_x = g_displays[i].origin_x;
        int32_t max_x = min_x + (int32_t)g_displays[i].width - 1;
        int32_t min_y = g_displays[i].origin_y;
        int32_t max_y = min_y + (int32_t)g_displays[i].height - 1;

        if (x >= min_x && x <= max_x && y >= min_y && y <= max_y) {
            return; // Point is valid inside an active monitor
        }
    }

    // If outside all monitors, clamp to primary display (index 0)
    int32_t p_min_x = g_displays[0].origin_x;
    int32_t p_max_x = p_min_x + (int32_t)g_displays[0].width - 1;
    int32_t p_min_y = g_displays[0].origin_y;
    int32_t p_max_y = p_min_y + (int32_t)g_displays[0].height - 1;

    if (x < p_min_x) x = p_min_x;
    if (x > p_max_x) x = p_max_x;
    if (y < p_min_y) y = p_min_y;
    if (y > p_max_y) y = p_max_y;

    *inout_x = x;
    *inout_y = y;
}

void pointer_bounds_update(uint32_t width, uint32_t height) {
    if (g_display_count > 0) {
        g_displays[0].width = width;
        g_displays[0].height = height;
    } else {
        pointer_bounds_init(width, height);
    }
}

void pointer_bounds_get_max(int32_t* out_max_x, int32_t* out_max_y) {
    if (g_display_count == 0) {
        if (out_max_x) *out_max_x = 0;
        if (out_max_y) *out_max_y = 0;
        return;
    }
    if (out_max_x) *out_max_x = g_displays[0].origin_x + (int32_t)g_displays[0].width - 1;
    if (out_max_y) *out_max_y = g_displays[0].origin_y + (int32_t)g_displays[0].height - 1;
}
