#ifndef ATOMS_APP_RUNTIME_H
#define ATOMS_APP_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Native Application Runtime & Event System
// ============================================================

#define ATOMS_APP_EVENT_QUEUE_SIZE 64
#define ATOMS_MAX_APP_TIMERS       16

typedef enum {
    ATOMS_EVENT_NONE = 0,
    ATOMS_EVENT_WINDOW_CREATE,
    ATOMS_EVENT_WINDOW_CLOSE,
    ATOMS_EVENT_MOUSE_CLICK,
    ATOMS_EVENT_KEY_PRESS,
    ATOMS_EVENT_TIMER,
    ATOMS_EVENT_APP_SIGNAL
} ATOMS_EventType;

typedef struct {
    ATOMS_EventType type;
    uint32_t        app_id;
    uint32_t        window_id;
    int32_t         param1;
    int32_t         param2;
    void*           data_ptr;
} ATOMS_Event;

typedef struct {
    uint32_t timer_id;
    uint32_t app_id;
    uint32_t interval_ms;
    uint32_t last_tick;
    bool     active;
    void     (*callback)(uint32_t timer_id);
} ATOMS_AppTimer;

// Core Runtime Interface
void ATOMS_Runtime_Init(void);
bool ATOMS_Runtime_PushEvent(const ATOMS_Event* ev);
bool ATOMS_Runtime_PollEvent(uint32_t app_id, ATOMS_Event* out_ev);
uint32_t ATOMS_Runtime_CreateTimer(uint32_t app_id, uint32_t interval_ms, void (*callback)(uint32_t));
void ATOMS_Runtime_CancelTimer(uint32_t timer_id);
void ATOMS_Runtime_UpdateTimers(uint32_t current_tick_ms);

// Crash Isolation Framework
void ATOMS_Runtime_HandleCrash(uint32_t app_id, const char* reason);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_APP_RUNTIME_H
