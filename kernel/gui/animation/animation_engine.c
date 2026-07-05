#include "animation_engine.h"
#include "kernel/core/lib/include/string.h"

#define MAX_ANIMATIONS 64

static BOFLOW_Animation g_animations[MAX_ANIMATIONS];
static uint32_t g_next_anim_id = 1;
static bool g_engine_initialized = false;

void boflow_init(void) {
    if (g_engine_initialized) return;
    
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        g_animations[i].id = 0;
        g_animations[i].is_running = false;
    }
    g_engine_initialized = true;
}

uint32_t animation_start(AnimType type, int32_t start_val, int32_t end_val, uint32_t duration_ms, AnimEasing easing) {
    if (!g_engine_initialized) boflow_init();
    
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (!g_animations[i].is_running) {
            uint32_t id = g_next_anim_id++;
            g_animations[i].id = id;
            g_animations[i].type = type;
            g_animations[i].easing = easing;
            g_animations[i].start_val = start_val;
            g_animations[i].end_val = end_val;
            g_animations[i].current_val = start_val;
            g_animations[i].duration_ms = duration_ms;
            g_animations[i].elapsed_ms = 0;
            g_animations[i].is_running = true;
            g_animations[i].on_update = 0; // NULL
            g_animations[i].on_complete = 0; // NULL
            g_animations[i].user_data = 0; // NULL
            
            return id;
        }
    }
    return 0; // Max animations reached
}

void animation_stop(uint32_t id) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (g_animations[i].id == id && g_animations[i].is_running) {
            g_animations[i].is_running = false;
            // Optionally trigger on_complete? Standard engines usually don't on explicit stop.
            break;
        }
    }
}

void animation_pause(uint32_t id) {
}

void animation_resume(uint32_t id) {
}

void animation_set_update_callback(uint32_t id, AnimUpdateCallback cb, void* user_data) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (g_animations[i].id == id) {
            g_animations[i].on_update = cb;
            g_animations[i].user_data = user_data;
            break;
        }
    }
}

void animation_set_complete_callback(uint32_t id, AnimCompleteCallback cb, void* user_data) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (g_animations[i].id == id) {
            g_animations[i].on_complete = cb;
            if (user_data) g_animations[i].user_data = user_data;
            break;
        }
    }
}

bool animation_is_running(uint32_t id) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (g_animations[i].id == id) {
            return g_animations[i].is_running;
        }
    }
    return false;
}

BOFLOW_Animation* animation_get(uint32_t id) {
    for (int i = 0; i < MAX_ANIMATIONS; i++) {
        if (g_animations[i].id == id) {
            return &g_animations[i];
        }
    }
    return 0;
}

// Internal accessor for scheduler
BOFLOW_Animation* _boflow_get_all_animations(void) {
    return g_animations;
}

uint32_t _boflow_get_max_animations(void) {
    return MAX_ANIMATIONS;
}
