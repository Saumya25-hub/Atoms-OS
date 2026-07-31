#include "framework/include/bos_ui_animation.h"
#include "kernel/core/lib/include/string.h"

#define BOS_MAX_UI_ANIMATIONS 32U
static BOS_UIAnimation g_active_animations[BOS_MAX_UI_ANIMATIONS];

void BOS_Animation_Init(void) {
    memset(g_active_animations, 0, sizeof(g_active_animations));
}

void BOS_Animation_Start(BOS_UIAnimation* anim) {
    if (!anim || !anim->target || anim->duration_ms == 0) return;

    for (uint32_t i = 0; i < BOS_MAX_UI_ANIMATIONS; i++) {
        if (!g_active_animations[i].is_active) {
            g_active_animations[i] = *anim;
            g_active_animations[i].current_val = anim->start_val;
            g_active_animations[i].elapsed_ms = 0;
            g_active_animations[i].is_active = true;
            break;
        }
    }
}

void BOS_Animation_Tick(uint32_t delta_ms) {
    for (uint32_t i = 0; i < BOS_MAX_UI_ANIMATIONS; i++) {
        if (g_active_animations[i].is_active) {
            BOS_UIAnimation* anim = &g_active_animations[i];
            anim->elapsed_ms += delta_ms;

            if (anim->elapsed_ms >= anim->duration_ms) {
                anim->current_val = anim->end_val;
                anim->is_active = false;
            } else {
                float progress = (float)anim->elapsed_ms / (float)anim->duration_ms;
                anim->current_val = anim->start_val + (anim->end_val - anim->start_val) * progress;
            }

            if (anim->on_update) {
                anim->on_update(anim->target, anim->current_val);
            }
        }
    }
}
