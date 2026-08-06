/**
 * @file cursor_animation.c
 * @brief Cursor Animation Timeline & Frame Scheduler Implementation
 */

#include "../include/bos_cursor_animation.h"
#include "../include/bos_cursor_hal.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/step14_telemetry.h"

static bce_anim_state_t g_anim_state = {0};

void bos_cursor_anim_init(void) {
    memset(&g_anim_state, 0, sizeof(bce_anim_state_t));
}

void bos_cursor_anim_start(bce_ani_t* ani) {
    if (!ani) return;
    g_anim_state.active_ani = ani;
    g_anim_state.current_step = 0;
    g_anim_state.last_frame_time_us = step14_rdtsc();
    g_anim_state.is_playing = true;
}

void bos_cursor_anim_stop(void) {
    g_anim_state.is_playing = false;
    g_anim_state.active_ani = NULL;
}

void bos_cursor_anim_tick(uint64_t now_us) {
    if (!g_anim_state.is_playing || !g_anim_state.active_ani) return;

    bce_ani_t* ani = g_anim_state.active_ani;
    if (ani->step_count == 0) return;

    uint32_t step = g_anim_state.current_step;
    uint32_t delay_ms = ani->step_rates_ms ? ani->step_rates_ms[step] : 100;
    uint64_t delay_us = (uint64_t)delay_ms * 1000U;

    if (now_us - g_anim_state.last_frame_time_us >= delay_us) {
        g_anim_state.current_step = (step + 1) % ani->step_count;
        g_anim_state.last_frame_time_us = now_us;

        bce_frame_t* next_frame = bos_cursor_anim_get_current_frame();
        if (next_frame) {
            bos_cursor_hal_upload_frame(next_frame);
        }
    }
}

bce_frame_t* bos_cursor_anim_get_current_frame(void) {
    if (!g_anim_state.active_ani || !g_anim_state.active_ani->cursor) return NULL;
    bce_ani_t* ani = g_anim_state.active_ani;
    uint32_t frame_idx = ani->seq_indices ? ani->seq_indices[g_anim_state.current_step] : g_anim_state.current_step;
    if (frame_idx >= ani->cursor->frame_count) frame_idx = 0;
    return &ani->cursor->frames[frame_idx];
}
