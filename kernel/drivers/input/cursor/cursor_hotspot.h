#ifndef ATOMS_OS_INPUT_CURSOR_HOTSPOT_H
#define ATOMS_OS_INPUT_CURSOR_HOTSPOT_H

/**
 * @file cursor_hotspot.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Hotspot & Bounding Box Engine
 * @section PURPOSE
 * Calculates exact top-left sprite rendering coordinates from pointer kinematics and hotspot offsets.
 * Computes screen-clamped drawing boxes and dirty rectangle unions for shadow buffer restoration.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Clamped Drawing Bounding Box --- */
typedef struct {
    int32_t screen_x;         /* Ideal top-left X on screen (can be < 0 if left-clipped) */
    int32_t screen_y;         /* Ideal top-left Y on screen (can be < 0 if top-clipped) */
    int32_t draw_x;           /* Clamped X on screen (>= 0 and < screen_w) */
    int32_t draw_y;           /* Clamped Y on screen (>= 0 and < screen_h) */
    uint32_t sprite_offset_x; /* Starting pixel column in sprite bitmap (0 if not left-clipped) */
    uint32_t sprite_offset_y; /* Starting pixel row in sprite bitmap (0 if not top-clipped) */
    uint32_t draw_w;          /* Clamped width to copy/draw */
    uint32_t draw_h;          /* Clamped height to copy/draw */
    bool is_valid;            /* True if at least 1 pixel is on screen */
} CursorBoundingBox;

/* --- Core Hotspot & Bounding Math --- */
void cursor_hotspot_calculate_box(
    int32_t ptr_x, int32_t ptr_y,
    uint32_t sprite_w, uint32_t sprite_h,
    uint32_t hotspot_x, uint32_t hotspot_y,
    uint32_t scale_percent,
    uint32_t screen_w, uint32_t screen_h,
    CursorBoundingBox* out_box
);

/* --- Dirty Rectangle Intersection & Union --- */
bool cursor_hotspot_boxes_intersect(const CursorBoundingBox* a, const CursorBoundingBox* b);
void cursor_hotspot_union_box(const CursorBoundingBox* a, const CursorBoundingBox* b, CursorBoundingBox* out_union);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_HOTSPOT_H
