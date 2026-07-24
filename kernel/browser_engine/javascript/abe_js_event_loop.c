#include "abe_js_event_loop.h"
#include "abe_js_promise.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

#define ABE_JS_MAX_TIMERS 64

static ABE_JSTimer g_timers[ABE_JS_MAX_TIMERS];
static uint32_t g_next_timer_id = 5000;
static bool g_js_event_loop_initialized = false;

ABE_Error ABE_JSEventLoop_Init(void) {
    memset(g_timers, 0, sizeof(g_timers));
    g_js_event_loop_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSEVENTLOOP", "ABE Browser Event Loop & Timer Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSEventLoop_Shutdown(void) {
    g_js_event_loop_initialized = false;
    return ABE_SUCCESS;
}

ABE_JSTimerID ABE_SetTimeout(ABE_TimerCallback cb, uint32_t delay_ms, void* user_data) {
    if (!g_js_event_loop_initialized || !cb) return ABE_INVALID_HANDLE;

    for (uint32_t i = 0; i < ABE_JS_MAX_TIMERS; i++) {
        if (!g_timers[i].in_use) {
            ABE_JSTimer* t = &g_timers[i];
            memset(t, 0, sizeof(ABE_JSTimer));
            t->id = (g_next_timer_id++) | (i << 16);
            t->cb = cb;
            t->user_data = user_data;
            t->delay_ms = delay_ms;
            t->is_interval = false;
            t->in_use = true;
            return t->id;
        }
    }
    return ABE_INVALID_HANDLE;
}

ABE_Error ABE_ClearTimeout(ABE_JSTimerID timer_id) {
    if (!g_js_event_loop_initialized || timer_id == ABE_INVALID_HANDLE) return ABE_ERR_INVALID_PARAM;
    uint32_t slot = (timer_id >> 16) & 0xFFFF;
    if (slot >= ABE_JS_MAX_TIMERS) return ABE_ERR_INVALID_PARAM;

    if (g_timers[slot].id == timer_id && g_timers[slot].in_use) {
        g_timers[slot].in_use = false;
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_JSTimerID ABE_SetInterval(ABE_TimerCallback cb, uint32_t delay_ms, void* user_data) {
    ABE_JSTimerID tid = ABE_SetTimeout(cb, delay_ms, user_data);
    if (tid != ABE_INVALID_HANDLE) {
        uint32_t slot = (tid >> 16) & 0xFFFF;
        g_timers[slot].is_interval = true;
    }
    return tid;
}

ABE_Error ABE_ClearInterval(ABE_JSTimerID timer_id) {
    return ABE_ClearTimeout(timer_id);
}

ABE_Error ABE_JSEventLoop_RunTick(ABE_JSContextHandle ctx_handle) {
    if (!g_js_event_loop_initialized) return ABE_ERR_NOT_INITIALIZED;

    // 1. Process Microtasks (Promises)
    ABE_JS_ProcessMicrotasks();

    // 2. Process Macrotasks (Timers)
    for (uint32_t i = 0; i < ABE_JS_MAX_TIMERS; i++) {
        if (g_timers[i].in_use && g_timers[i].cb) {
            g_timers[i].cb(g_timers[i].user_data);
            if (!g_timers[i].is_interval) {
                g_timers[i].in_use = false;
            }
        }
    }

    ABE_Diag_RecordJSEventLoopTick();
    return ABE_SUCCESS;
}
