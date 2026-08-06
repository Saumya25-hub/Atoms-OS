/**
 * @file bos_cursor_animation.h
 * @brief Cursor Animation Timeline & Frame Scheduler Engine
 */

#ifndef BOS_CURSOR_ANIMATION_H
#define BOS_CURSOR_ANIMATION_H

#include "bos_cursor.h"
#include "bos_ani_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bce_ani_t* active_ani;
    uint32_t   current_step;
    uint64_t   last_frame_time_us;
    bool       is_playing;
} bce_anim_state_t;

void bos_cursor_anim_init(void);
void bos_cursor_anim_start(bce_ani_t* ani);
void bos_cursor_anim_stop(void);
void bos_cursor_anim_tick(uint64_t now_us);
bce_frame_t* bos_cursor_anim_get_current_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CURSOR_ANIMATION_H */
