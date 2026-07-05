#ifndef ANIMATION_ENGINE_H
#define ANIMATION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/gui/animation/animation_easing.h"

// Types of animations BOFLOW supports
typedef enum {
    ANIM_TYPE_FADE_IN,
    ANIM_TYPE_FADE_OUT,
    ANIM_TYPE_CROSS_FADE,
    ANIM_TYPE_OPACITY,
    ANIM_TYPE_WALLPAPER_TRANSITION,
    ANIM_TYPE_WINDOW_ALPHA,
    ANIM_TYPE_CUSTOM
} AnimType;

// Callback definitions (current_val is fixed-point or integer, depending on context)
typedef void (*AnimUpdateCallback)(uint32_t anim_id, int32_t current_val, void* user_data);
typedef void (*AnimCompleteCallback)(uint32_t anim_id, void* user_data);

typedef struct {
    uint32_t id;
    uint32_t target_id; // e.g., window ID or surface identifier
    AnimType type;
    AnimEasing easing;
    int32_t start_val;
    int32_t end_val;
    int32_t current_val;
    uint32_t duration_ms;
    uint32_t elapsed_ms;
    bool is_running;
    AnimUpdateCallback on_update;
    AnimCompleteCallback on_complete;
    void* user_data;
} BOFLOW_Animation;

// Engine Core APIs
void boflow_init(void);
uint32_t animation_start(AnimType type, int32_t start_val, int32_t end_val, uint32_t duration_ms, AnimEasing easing);
void animation_stop(uint32_t id);
void animation_pause(uint32_t id);
void animation_resume(uint32_t id);
void animation_set_update_callback(uint32_t id, AnimUpdateCallback cb, void* user_data);
void animation_set_complete_callback(uint32_t id, AnimCompleteCallback cb, void* user_data);
bool animation_is_running(uint32_t id);
BOFLOW_Animation* animation_get(uint32_t id);

#endif
