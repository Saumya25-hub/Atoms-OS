#include "../include/bar_api.h"

typedef struct {
    BARTimerID  timer_id;
    BARWindowID window_id;
    uint32_t    interval_ms;
    bool        active;
} BARTimerEntry;

static BARTimerEntry g_timers[BAR_MAX_TIMERS];

BARTimerID BAR_SetTimer(BARWindowID win_id, uint32_t interval_ms) {
    if (win_id == 0 || interval_ms == 0) return 0;
    
    for (uint32_t i = 0; i < BAR_MAX_TIMERS; i++) {
        if (!g_timers[i].active) {
            g_timers[i].timer_id = i + 1;
            g_timers[i].window_id = win_id;
            g_timers[i].interval_ms = interval_ms;
            g_timers[i].active = true;
            return g_timers[i].timer_id;
        }
    }
    return 0;
}

int32_t BAR_KillTimer(BARTimerID timer_id) {
    if (timer_id == 0 || timer_id > BAR_MAX_TIMERS) return -1;
    uint32_t idx = timer_id - 1;
    if (!g_timers[idx].active) return -1;
    
    g_timers[idx].active = false;
    return 0;
}
