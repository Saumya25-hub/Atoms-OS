#include "app_runtime.h"
#include "kernel/application/app_manager/app_manager.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern void bwe_log(const char* level, const char* msg);

static ATOMS_Event    g_event_queue[ATOMS_APP_EVENT_QUEUE_SIZE];
static uint32_t       g_event_head = 0;
static uint32_t       g_event_tail = 0;
static uint32_t       g_event_count = 0;

static ATOMS_AppTimer g_app_timers[ATOMS_MAX_APP_TIMERS];
static uint32_t       g_next_timer_id = 1;

void ATOMS_Runtime_Init(void) {
    g_event_head = 0;
    g_event_tail = 0;
    g_event_count = 0;

    for (uint32_t i = 0; i < ATOMS_MAX_APP_TIMERS; i++) {
        g_app_timers[i].active = false;
        g_app_timers[i].timer_id = 0;
    }
    bwe_log("INFO", "ATOMS App Runtime Subsystem Initialized");
}

bool ATOMS_Runtime_PushEvent(const ATOMS_Event* ev) {
    if (!ev || g_event_count >= ATOMS_APP_EVENT_QUEUE_SIZE) return false;

    g_event_queue[g_event_tail] = *ev;
    g_event_tail = (g_event_tail + 1) % ATOMS_APP_EVENT_QUEUE_SIZE;
    g_event_count++;
    return true;
}

bool ATOMS_Runtime_PollEvent(uint32_t app_id, ATOMS_Event* out_ev) {
    if (g_event_count == 0 || !out_ev) return false;

    for (uint32_t i = 0; i < g_event_count; i++) {
        uint32_t idx = (g_event_head + i) % ATOMS_APP_EVENT_QUEUE_SIZE;
        if (g_event_queue[idx].app_id == app_id || g_event_queue[idx].app_id == 0) {
            *out_ev = g_event_queue[idx];

            // Shift event queue entries down
            for (uint32_t j = i; j < g_event_count - 1; j++) {
                uint32_t cur = (g_event_head + j) % ATOMS_APP_EVENT_QUEUE_SIZE;
                uint32_t nxt = (g_event_head + j + 1) % ATOMS_APP_EVENT_QUEUE_SIZE;
                g_event_queue[cur] = g_event_queue[nxt];
            }
            g_event_tail = (g_event_tail + ATOMS_APP_EVENT_QUEUE_SIZE - 1) % ATOMS_APP_EVENT_QUEUE_SIZE;
            g_event_count--;
            return true;
        }
    }
    return false;
}

uint32_t ATOMS_Runtime_CreateTimer(uint32_t app_id, uint32_t interval_ms, void (*callback)(uint32_t)) {
    for (uint32_t i = 0; i < ATOMS_MAX_APP_TIMERS; i++) {
        if (!g_app_timers[i].active) {
            g_app_timers[i].timer_id = g_next_timer_id++;
            g_app_timers[i].app_id = app_id;
            g_app_timers[i].interval_ms = interval_ms;
            g_app_timers[i].last_tick = 0;
            g_app_timers[i].active = true;
            g_app_timers[i].callback = callback;
            return g_app_timers[i].timer_id;
        }
    }
    return 0;
}

void ATOMS_Runtime_CancelTimer(uint32_t timer_id) {
    for (uint32_t i = 0; i < ATOMS_MAX_APP_TIMERS; i++) {
        if (g_app_timers[i].active && g_app_timers[i].timer_id == timer_id) {
            g_app_timers[i].active = false;
            break;
        }
    }
}

void ATOMS_Runtime_UpdateTimers(uint32_t current_tick_ms) {
    for (uint32_t i = 0; i < ATOMS_MAX_APP_TIMERS; i++) {
        if (g_app_timers[i].active) {
            if (current_tick_ms - g_app_timers[i].last_tick >= g_app_timers[i].interval_ms) {
                g_app_timers[i].last_tick = current_tick_ms;
                if (g_app_timers[i].callback) {
                    g_app_timers[i].callback(g_app_timers[i].timer_id);
                }
                ATOMS_Event ev;
                ev.type = ATOMS_EVENT_TIMER;
                ev.app_id = g_app_timers[i].app_id;
                ev.param1 = (int32_t)g_app_timers[i].timer_id;
                ev.param2 = 0;
                ev.data_ptr = 0;
                ATOMS_Runtime_PushEvent(&ev);
            }
        }
    }
}

void ATOMS_Runtime_HandleCrash(uint32_t app_id, const char* reason) {
    display_print("\n[CRASH_ISOLATION] Application ID ");
    display_print_dec(app_id);
    display_print(" crashed: ");
    display_print(reason ? reason : "Unknown Fault");
    display_print("\n[CRASH_ISOLATION] Cleaning up surface windows & isolating process.\n");

    ATOMS_StopApplication(app_id);
}
