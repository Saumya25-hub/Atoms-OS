#ifndef ATOMS_OS_INPUT_CURSOR_THEME_H
#define ATOMS_OS_INPUT_CURSOR_THEME_H

/**
 * @file cursor_theme.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Theme Registry
 * @section PURPOSE
 * Manages standard OS cursor shapes (Arrow, Text Beam, Resize, Busy, Wait, Crosshair, Hand)
 * and multi-frame animation sequences without heap allocation.
 */

#include <stdint.h>
#include <stdbool.h>
#include "cursor_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CURSOR_THEME_MAX_DIM 32
#define CURSOR_THEME_MAX_FRAMES 4

/* --- Theme Sprite Definition --- */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t frame_count;
    uint32_t frame_interval_ms;
    bool is_animated;
    uint32_t bitmaps[CURSOR_THEME_MAX_FRAMES][CURSOR_THEME_MAX_DIM * CURSOR_THEME_MAX_DIM];
} CursorThemeSprite;

/* --- Lifecycle & Initialization --- */
void cursor_theme_init(void);

/* --- Sprite Query API --- */
const uint32_t* cursor_theme_get_bitmap(
    CursorShape shape,
    uint32_t anim_frame,
    uint32_t* out_w,
    uint32_t* out_h,
    uint32_t* out_hx,
    uint32_t* out_hy
);

bool cursor_theme_is_animated(CursorShape shape, uint32_t* out_frame_count, uint32_t* out_interval_ms);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_THEME_H
