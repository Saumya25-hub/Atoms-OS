#include "../include/ame.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/lib/include/string.h"
#include <stddef.h>

#define AME_MAX_TRACKS 512

typedef struct {
    AME_State state;
    uint16_t generation;
    void* target;
    AME_PropertyID prop;
    int32_t start_val;
    int32_t end_val;
    int32_t current_val;
    uint64_t start_time_ms;
    uint32_t duration_ms;
    uint32_t delay_ms;
    AME_EasingCurve curve;
    uint32_t flags;
    AME_Callback on_update;
    void* update_user_data;
    AME_Callback on_complete;
    void* complete_user_data;
} AME_Track;

static AME_Track s_tracks[AME_MAX_TRACKS];
static uint32_t s_global_scale_percent = 100;
static bool s_reduced_motion = false;
static bool s_boot_experience_active = false;

void AME_Init(void) {
    memset(s_tracks, 0, sizeof(s_tracks));
    for (int i = 0; i < AME_MAX_TRACKS; i++) {
        s_tracks[i].generation = 1;
        s_tracks[i].state = AME_STATE_IDLE;
    }
    s_global_scale_percent = 100;
    s_reduced_motion = false;
    s_boot_experience_active = false;
}

static AME_Track* GetTrack(AME_Handle handle) {
    if (handle == AME_INVALID_HANDLE) return NULL;
    uint16_t idx = handle & 0xFFFF;
    uint16_t gen = (handle >> 16) & 0xFFFF;
    if (idx >= AME_MAX_TRACKS) return NULL;
    if (s_tracks[idx].generation != gen) return NULL;
    return &s_tracks[idx];
}

AME_Handle AME_CreateAnimation(void* target, AME_PropertyID prop, int32_t start_val, int32_t end_val, uint32_t duration_ms, AME_EasingCurve curve) {
    for (uint16_t i = 0; i < AME_MAX_TRACKS; i++) {
        if (s_tracks[i].state == AME_STATE_IDLE) {
            uint16_t gen = s_tracks[i].generation;
            if (gen == 0) gen = 1;
            
            memset(&s_tracks[i], 0, sizeof(AME_Track));
            s_tracks[i].generation = gen;
            s_tracks[i].state = AME_STATE_IDLE;
            s_tracks[i].target = target;
            s_tracks[i].prop = prop;
            s_tracks[i].start_val = start_val;
            s_tracks[i].end_val = end_val;
            s_tracks[i].current_val = start_val;
            s_tracks[i].duration_ms = duration_ms;
            s_tracks[i].curve = curve;
            s_tracks[i].flags = AME_FLAG_AUTO_FREE;
            
            return ((uint32_t)gen << 16) | i;
        }
    }
    return AME_INVALID_HANDLE;
}

void AME_Play(AME_Handle handle) {
    AME_Track* track = GetTrack(handle);
    if (!track) return;
    if (track->state == AME_STATE_RUNNING || track->state == AME_STATE_QUEUED) return;
    
    uint64_t now = timer_get_ticks();
    if (track->delay_ms > 0) {
        track->state = AME_STATE_QUEUED;
        track->start_time_ms = now;
    } else {
        track->state = AME_STATE_RUNNING;
        track->start_time_ms = now;
    }
}

void AME_Pause(AME_Handle handle) {
    AME_Track* track = GetTrack(handle);
    if (!track) return;
    if (track->state == AME_STATE_RUNNING) {
        track->state = AME_STATE_PAUSED;
    }
}

void AME_Cancel(AME_Handle handle) {
    AME_Track* track = GetTrack(handle);
    if (!track) return;
    track->state = AME_STATE_CANCELLED;
    if (track->flags & AME_FLAG_AUTO_FREE) {
        track->generation++;
        if (track->generation == 0) track->generation = 1;
        track->state = AME_STATE_IDLE;
    }
}

void AME_DestroyAnimation(AME_Handle handle) {
    AME_Track* track = GetTrack(handle);
    if (!track) return;
    track->generation++;
    if (track->generation == 0) track->generation = 1;
    track->state = AME_STATE_IDLE;
}

void AME_SetDelay(AME_Handle handle, uint32_t delay_ms) {
    AME_Track* track = GetTrack(handle);
    if (track) track->delay_ms = delay_ms;
}

void AME_SetCallback(AME_Handle handle, AME_Callback on_update, void* user_data) {
    AME_Track* track = GetTrack(handle);
    if (track) {
        track->on_update = on_update;
        track->update_user_data = user_data;
    }
}

void AME_SetOnComplete(AME_Handle handle, AME_Callback on_complete, void* user_data) {
    AME_Track* track = GetTrack(handle);
    if (track) {
        track->on_complete = on_complete;
        track->complete_user_data = user_data;
    }
}

void AME_SetFlags(AME_Handle handle, uint32_t flags) {
    AME_Track* track = GetTrack(handle);
    if (track) track->flags = flags;
}

int32_t AME_GetCurrentValue(AME_Handle handle) {
    AME_Track* track = GetTrack(handle);
    return track ? track->current_val : 0;
}

AME_State AME_GetState(AME_Handle handle) {
    AME_Track* track = GetTrack(handle);
    return track ? track->state : AME_STATE_IDLE;
}

void AME_SetGlobalScale(uint32_t scale_percent) {
    if (scale_percent == 0) scale_percent = 1;
    s_global_scale_percent = scale_percent;
}

void AME_SetReducedMotion(bool enabled) {
    s_reduced_motion = enabled;
}

bool AME_IsBootExperienceActive(void) {
    return s_boot_experience_active;
}

void AME_SetBootExperienceActive(bool active) {
    s_boot_experience_active = active;
}

void AME_Tick(uint64_t current_time_ms) {
    if (current_time_ms == 0) {
        current_time_ms = timer_get_ticks();
    }
    
    for (uint16_t i = 0; i < AME_MAX_TRACKS; i++) {
        AME_Track* track = &s_tracks[i];
        
        if (track->state == AME_STATE_QUEUED) {
            if (current_time_ms >= track->start_time_ms + track->delay_ms) {
                track->state = AME_STATE_RUNNING;
                track->start_time_ms = current_time_ms;
            }
        }
        
        if (track->state == AME_STATE_RUNNING) {
            uint32_t eff_duration = (track->duration_ms * s_global_scale_percent) / 100;
            int32_t progress = 0;
            
            if (s_reduced_motion || eff_duration == 0 || current_time_ms >= track->start_time_ms + eff_duration) {
                progress = 65536;
            } else {
                uint64_t elapsed = current_time_ms - track->start_time_ms;
                progress = (int32_t)((elapsed * 65536ULL) / eff_duration);
                if (progress > 65536) progress = 65536;
            }
            
            int32_t eased = AME_EvaluateCurve(track->curve, progress);
            int64_t diff = (int64_t)track->end_val - (int64_t)track->start_val;
            track->current_val = track->start_val + (int32_t)((diff * eased) >> 16);
            
            if (track->on_update) {
                track->on_update(((uint32_t)track->generation << 16) | i, track->update_user_data, track->current_val);
            }
            
            if (progress >= 65536) {
                track->current_val = track->end_val;
                track->state = AME_STATE_COMPLETED;
                
                if (track->on_complete) {
                    track->on_complete(((uint32_t)track->generation << 16) | i, track->complete_user_data, track->current_val);
                }
                
                if (track->flags & AME_FLAG_AUTO_FREE) {
                    track->generation++;
                    if (track->generation == 0) track->generation = 1;
                    track->state = AME_STATE_IDLE;
                }
            }
        }
    }
}
