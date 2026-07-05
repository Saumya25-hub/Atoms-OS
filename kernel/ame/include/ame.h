#ifndef ATOMS_AME_H
#define ATOMS_AME_H

#include <stdint.h>
#include <stdbool.h>

// Handle definitions (32-bit: [16-bit generation | 16-bit pool index])
typedef uint32_t AME_Handle;
#define AME_INVALID_HANDLE 0

// Property identifiers for animation targets
typedef enum {
    PROP_OPACITY = 1,
    PROP_X       = 2,
    PROP_Y       = 3,
    PROP_WIDTH   = 4,
    PROP_HEIGHT  = 5,
    PROP_SCALE   = 6
} AME_PropertyID;

// Parametric easing curves (16.16 fixed-point evaluation)
typedef enum {
    EASE_LINEAR       = 0,
    EASE_OUT_CUBIC    = 1,
    EASE_IN_OUT_CUBIC = 2,
    EASE_OUT_QUINT    = 3,
    EASE_OUT_BOUNCE   = 4,
    EASE_OUT_ELASTIC  = 5
} AME_EasingCurve;

// Track state machine
typedef enum {
    AME_STATE_IDLE      = 0,
    AME_STATE_QUEUED    = 1,
    AME_STATE_RUNNING   = 2,
    AME_STATE_PAUSED    = 3,
    AME_STATE_COMPLETED = 4,
    AME_STATE_CANCELLED = 5
} AME_State;

// Configuration flags
#define AME_FLAG_AUTO_FREE       (1 << 0)
#define AME_FLAG_REDUCED_MOTION  (1 << 1)

// Spatial bounding rectangle for dirty rect union tracking
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} AME_Rect;

// Animation callback hooks
typedef void (*AME_Callback)(AME_Handle handle, void* user_data, int32_t current_value);

// Core System Service Initialization & Lifecycle
void AME_Init(void);
void AME_Tick(uint64_t current_time_ms);

// Animation Creation & Control
AME_Handle AME_CreateAnimation(void* target, AME_PropertyID prop, int32_t start_val, int32_t end_val, uint32_t duration_ms, AME_EasingCurve curve);
void AME_Play(AME_Handle handle);
void AME_Pause(AME_Handle handle);
void AME_Cancel(AME_Handle handle);
void AME_DestroyAnimation(AME_Handle handle);
void AME_SetDelay(AME_Handle handle, uint32_t delay_ms);
void AME_SetCallback(AME_Handle handle, AME_Callback on_update, void* user_data);
void AME_SetOnComplete(AME_Handle handle, AME_Callback on_complete, void* user_data);
void AME_SetFlags(AME_Handle handle, uint32_t flags);
int32_t AME_GetCurrentValue(AME_Handle handle);
AME_State AME_GetState(AME_Handle handle);

// Global Developer / Accessibility Controls
void AME_SetGlobalScale(uint32_t scale_percent); // 100 = 1.0x (normal), 50 = 0.5x (fast), 200 = 2.0x (slow debug)
void AME_SetReducedMotion(bool enabled);         // When true, animations complete instantly
bool AME_IsBootExperienceActive(void);
void AME_SetBootExperienceActive(bool active);

// Internal Easing Math Prototype (Fixed-Point 16.16: 1.0 = 65536)
int32_t AME_EvaluateCurve(AME_EasingCurve curve, int32_t progress_fixed16);

#endif // ATOMS_AME_H
