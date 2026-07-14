/**
 * @file cursor_hotspot.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Hotspot & Bounding Box Implementation
 * @section PURPOSE
 * Deterministic bounding box arithmetic and screen clipping without floating-point math or heap allocation.
 */

#include "cursor_hotspot.h"
#include <stddef.h>
#include "kernel/drivers/display/display.h"

void cursor_hotspot_calculate_box(
    int32_t ptr_x, int32_t ptr_y,
    uint32_t sprite_w, uint32_t sprite_h,
    uint32_t hotspot_x, uint32_t hotspot_y,
    uint32_t scale_percent,
    uint32_t screen_w, uint32_t screen_h,
    CursorBoundingBox* out_box
) {
    if (!out_box) return;
    
    out_box->is_valid = false;
    if (sprite_w == 0 || sprite_h == 0 || screen_w == 0 || screen_h == 0) return;
    if (scale_percent < 100) scale_percent = 100;

    /* 1. Calculate scaled sprite dimensions and scaled hotspot */
    uint32_t scaled_w = (sprite_w * scale_percent) / 100;
    uint32_t scaled_h = (sprite_h * scale_percent) / 100;
    uint32_t scaled_hx = (hotspot_x * scale_percent) / 100;
    uint32_t scaled_hy = (hotspot_y * scale_percent) / 100;
    
    if (scaled_w == 0) scaled_w = 1;
    if (scaled_h == 0) scaled_h = 1;

    /* 2. Calculate ideal top-left screen coordinate */
    int32_t sx = ptr_x - (int32_t)scaled_hx;
    int32_t sy = ptr_y - (int32_t)scaled_hy;
    
    out_box->screen_x = sx;
    out_box->screen_y = sy;

    /* 3. Check completely off-screen */
    if (sx + (int32_t)scaled_w <= 0 || sx >= (int32_t)screen_w ||
        sy + (int32_t)scaled_h <= 0 || sy >= (int32_t)screen_h) {
        out_box->draw_x = 0;
        out_box->draw_y = 0;
        out_box->sprite_offset_x = 0;
        out_box->sprite_offset_y = 0;
        out_box->draw_w = 0;
        out_box->draw_h = 0;
        out_box->is_valid = false;
        return;
    }

    /* 4. Left/Top Clipping */
    int32_t dx = sx;
    int32_t dy = sy;
    uint32_t off_x = 0;
    uint32_t off_y = 0;
    uint32_t dw = scaled_w;
    uint32_t dh = scaled_h;

    if (dx < 0) {
        off_x = (uint32_t)(-dx);
        if (off_x >= dw) dw = 0;
        else dw -= off_x;
        dx = 0;
    }
    if (dy < 0) {
        off_y = (uint32_t)(-dy);
        if (off_y >= dh) dh = 0;
        else dh -= off_y;
        dy = 0;
    }

    /* 5. Right/Bottom Clipping */
    if (dx + (int32_t)dw > (int32_t)screen_w) {
        if ((int32_t)screen_w > dx) {
            dw = (uint32_t)((int32_t)screen_w - dx);
        } else {
            dw = 0;
        }
    }
    if (dy + (int32_t)dh > (int32_t)screen_h) {
        if ((int32_t)screen_h > dy) {
            dh = (uint32_t)((int32_t)screen_h - dy);
        } else {
            dh = 0;
        }
    }

    if (dw == 0 || dh == 0) {
        out_box->is_valid = false;
        return;
    }

    out_box->draw_x = dx;
    out_box->draw_y = dy;
    out_box->sprite_offset_x = (off_x * 100) / scale_percent; /* Convert back to unscaled sprite coord */
    out_box->sprite_offset_y = (off_y * 100) / scale_percent;
    out_box->draw_w = dw;
    out_box->draw_h = dh;
    out_box->is_valid = true;
    
    bool clamped = (out_box->draw_x != out_box->screen_x || out_box->draw_y != out_box->screen_y ||
                    out_box->draw_w != scaled_w || out_box->draw_h != scaled_h);
    // display_print("HOTSPOT\ninput x="); display_print_dec(ptr_x);
    // display_print("\ninput y="); display_print_dec(ptr_y);
    // display_print("\noutput box.x="); display_print_dec(out_box->draw_x);
    // display_print("\noutput box.y="); display_print_dec(out_box->draw_y);
    // display_print("\nclamped="); display_print(clamped ? "yes\n" : "no\n");
}

bool cursor_hotspot_boxes_intersect(const CursorBoundingBox* a, const CursorBoundingBox* b) {
    if (!a || !b || !a->is_valid || !b->is_valid) return false;
    
    int32_t a_right = a->draw_x + (int32_t)a->draw_w;
    int32_t a_bottom = a->draw_y + (int32_t)a->draw_h;
    int32_t b_right = b->draw_x + (int32_t)b->draw_w;
    int32_t b_bottom = b->draw_y + (int32_t)b->draw_h;

    if (a_right <= b->draw_x || a->draw_x >= b_right) return false;
    if (a_bottom <= b->draw_y || a->draw_y >= b_bottom) return false;
    return true;
}

void cursor_hotspot_union_box(const CursorBoundingBox* a, const CursorBoundingBox* b, CursorBoundingBox* out_union) {
    if (!out_union) return;
    if (!a || !a->is_valid) {
        if (b && b->is_valid) *out_union = *b;
        else out_union->is_valid = false;
        return;
    }
    if (!b || !b->is_valid) {
        *out_union = *a;
        return;
    }

    int32_t min_x = (a->draw_x < b->draw_x) ? a->draw_x : b->draw_x;
    int32_t min_y = (a->draw_y < b->draw_y) ? a->draw_y : b->draw_y;
    
    int32_t a_right = a->draw_x + (int32_t)a->draw_w;
    int32_t b_right = b->draw_x + (int32_t)b->draw_w;
    int32_t max_right = (a_right > b_right) ? a_right : b_right;
    
    int32_t a_bottom = a->draw_y + (int32_t)a->draw_h;
    int32_t b_bottom = b->draw_y + (int32_t)b->draw_h;
    int32_t max_bottom = (a_bottom > b_bottom) ? a_bottom : b_bottom;

    out_union->screen_x = min_x;
    out_union->screen_y = min_y;
    out_union->draw_x = min_x;
    out_union->draw_y = min_y;
    out_union->sprite_offset_x = 0;
    out_union->sprite_offset_y = 0;
    out_union->draw_w = (uint32_t)(max_right - min_x);
    out_union->draw_h = (uint32_t)(max_bottom - min_y);
    out_union->is_valid = true;
}
