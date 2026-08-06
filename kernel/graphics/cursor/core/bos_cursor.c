/**
 * @file bos_cursor.c
 * @brief BOS Cursor Engine (BCE) V1.0 Master Implementation
 */

#include "../include/bos_cursor.h"
#include "../include/bos_cursor_cache.h"
#include "../include/bos_cursor_theme.h"
#include "../include/bos_cursor_animation.h"
#include "../include/bos_cursor_hal.h"
#include "../include/bos_cursor_diag.h"

extern uint64_t timer_get_ticks(void);

static bce_cursor_t* g_active_cursor = NULL;
static uint64_t g_anim_start_ms = 0;
static uint32_t g_anim_timeout_ms = 0;
static uint32_t g_current_frame_idx = 0;

bce_error_t bos_cursor_subsystem_init(void) {
    bos_cursor_diag_init();
    bos_cursor_cache_init();
    bos_cursor_theme_init();
    bos_cursor_anim_init();
    bos_cursor_hal_init();

    g_anim_start_ms = timer_get_ticks();
    g_anim_timeout_ms = 0;
    g_current_frame_idx = 0;

    /* Set default ARROW cursor (arrow.cur) */
    g_active_cursor = bos_cursor_theme_get_type(BCE_CURSOR_ARROW);
    if (g_active_cursor && g_active_cursor->frame_count > 0 && g_active_cursor->frames) {
        bos_cursor_hal_upload_frame(&g_active_cursor->frames[0]);
    }
    return BCE_OK;
}

void bos_cursor_subsystem_shutdown(void) {
    bos_cursor_anim_stop();
    bos_cursor_theme_shutdown();
    bos_cursor_cache_shutdown();
    g_active_cursor = NULL;
    g_anim_timeout_ms = 0;
}

bce_error_t bos_cursor_set_active_type(bce_cursor_type_t type) {
    bce_cursor_t* cur = bos_cursor_theme_get_type(type);
    if (!cur) return BCE_ERR_NOT_FOUND;

    g_active_cursor = cur;
    g_current_frame_idx = 0;
    g_anim_start_ms = timer_get_ticks();

    if (type == BCE_CURSOR_APPSTARTING || type == BCE_CURSOR_WAIT) {
        g_anim_timeout_ms = 3500; /* 3.5 seconds smooth animated loading spinner */
    } else {
        g_anim_timeout_ms = 0;
    }

    if (cur->frame_count > 0 && cur->frames) {
        bos_cursor_hal_upload_frame(&cur->frames[0]);
    }
    return BCE_OK;
}

bce_error_t bos_cursor_set_custom(bce_cursor_t* cursor) {
    if (!cursor) return BCE_ERR_INVALID_PARAM;
    g_active_cursor = cursor;
    g_current_frame_idx = 0;
    g_anim_start_ms = timer_get_ticks();
    g_anim_timeout_ms = 0;

    if (cursor->frame_count > 0 && cursor->frames) {
        bos_cursor_hal_upload_frame(&cursor->frames[0]);
    }
    return BCE_OK;
}

void bos_cursor_tick(void) {
    if (!g_active_cursor || !g_active_cursor->frames) {
        return;
    }

    uint64_t now_ms = timer_get_ticks();

    if (g_anim_timeout_ms > 0) {
        uint32_t elapsed_ms = (uint32_t)(now_ms - g_anim_start_ms);
        if (elapsed_ms >= g_anim_timeout_ms) {
            g_anim_timeout_ms = 0;
            bos_cursor_set_active_type(BCE_CURSOR_ARROW);
            return;
        }
    }

    if (g_active_cursor->frame_count > 1) {
        uint32_t frame_delay_ms = 40; /* 25 FPS smooth spin animation */
        uint32_t elapsed_ms = (uint32_t)(now_ms - g_anim_start_ms);
        uint32_t frame_idx = (elapsed_ms / frame_delay_ms) % g_active_cursor->frame_count;

        if (frame_idx != g_current_frame_idx) {
            g_current_frame_idx = frame_idx;
            bos_cursor_hal_upload_frame(&g_active_cursor->frames[frame_idx]);
        }
    }
}

bce_cursor_t* bos_cursor_get_current(void) {
    return g_active_cursor;
}
