/**
 * @file cursor_animation.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Animation Scheduler Implementation
 * @section PURPOSE
 * Advances multi-frame cursor sequences based on timer ticks without blocking.
 */

#include "cursor_animation.h"
#include "cursor_state.h"
#include "cursor_theme.h"
#include <stddef.h>

extern uint32_t timer_get_ticks(void);

static uint32_t g_last_tick_ms = 0;
static CursorShape g_last_shape = CURSOR_SHAPE_ARROW;

void cursor_animation_init(void) {
    g_last_tick_ms = timer_get_ticks();
    g_last_shape = CURSOR_SHAPE_ARROW;
}

void cursor_animation_reset(void) {
    g_last_tick_ms = timer_get_ticks();
    cursor_state_set_anim_frame(0);
}

bool cursor_animation_tick(void) {
    CursorShape shape = cursor_state_get_shape();
    
    /* If shape changed, reset animation sequence */
    if (shape != g_last_shape) {
        g_last_shape = shape;
        g_last_tick_ms = timer_get_ticks();
        return false;
    }

    uint32_t frame_count = 1;
    uint32_t interval_ms = 0;
    if (!cursor_theme_is_animated(shape, &frame_count, &interval_ms)) {
        return false;
    }

    if (frame_count <= 1 || interval_ms == 0) return false;

    uint32_t now = timer_get_ticks();
    if (now - g_last_tick_ms >= interval_ms) {
        g_last_tick_ms = now;
        uint32_t cur_frame = cursor_state_get_anim_frame();
        uint32_t next_frame = (cur_frame + 1) % frame_count;
        cursor_state_set_anim_frame(next_frame);
        return true;
    }

    return false;
}
