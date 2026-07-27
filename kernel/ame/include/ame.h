#ifndef ATOMS_AME_H
#define ATOMS_AME_H

#include <stdint.h>
#include <stdbool.h>

// Handle definitions (32-bit: [16-bit generation | 16-bit pool index])
typedef uint32_t AME_Handle;
#define AME_INVALID_HANDLE 0

// Property identifiers for animation targets
typedef enum {
    PROP_OPACITY  = 1,
    PROP_X        = 2,
    PROP_Y        = 3,
    PROP_WIDTH    = 4,
    PROP_HEIGHT   = 5,
    PROP_SCALE    = 6,
    PROP_ROTATION = 7
} AME_PropertyID;

// Parametric easing curves (16.16 fixed-point evaluation)
typedef enum {
    EASE_LINEAR       = 0,
    EASE_OUT_CUBIC    = 1,
    EASE_IN_OUT_CUBIC = 2,
    EASE_OUT_QUINT    = 3,
    EASE_OUT_BOUNCE   = 4,
    EASE_OUT_ELASTIC  = 5,
    EASE_IN_OUT_SINE  = 6
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
#define AME_FLAG_LOOP            (1 << 2)

// Spatial bounding rectangle for dirty rect union tracking
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} AME_Rect;

// Animation callback hooks
typedef void (*AME_Callback)(AME_Handle handle, void* user_data, int32_t current_value);

// ----------------------------------------------------
// AME SPINNER MODULE (Windows 11 / Linux Fluid Spinner)
// ----------------------------------------------------
typedef struct {
    int center_x;
    int center_y;
    int radius;            /* Circle radius (e.g. 18px) */
    int dot_radius;        /* Individual dot radius (e.g. 3px) */
    int num_dots;          /* Number of dots (e.g. 12) */
    uint32_t color;        /* Primary color (0x00FFFFFF) */
    uint8_t alpha;         /* Master opacity (0..255) */
    uint64_t elapsed_ms;   /* Time from Animation Clock */
    float speed_scale;     /* Configurable rotation speed */
    uint32_t base_angle;   /* Primary rotation angle (0..359 deg) */
    bool active;
} AME_Spinner;

// Core System Service Initialization & Master Loop
void AME_Init(void);
void AME_Update(uint64_t delta_ms);
void AME_Tick(uint64_t current_time_ms);
uint64_t AME_GetClockTime(void);

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

// Internal Easing Math & Interpolation (Fixed-Point 16.16: 1.0 = 65536)
int32_t AME_EvaluateCurve(AME_EasingCurve curve, int32_t progress_fixed16);
int32_t AME_Interpolate(int32_t start, int32_t end, int32_t progress_fixed16);

// ----------------------------------------------------
// AME Subsystem Modules
// ----------------------------------------------------

// Fade Module
int32_t AME_Fade_Calculate(uint64_t elapsed_ms, uint32_t duration_ms, uint8_t start_alpha, uint8_t end_alpha, AME_EasingCurve curve);

// Scale Module
int32_t AME_Scale_Calculate(uint64_t elapsed_ms, uint32_t duration_ms, int32_t start_scale_fixed16, int32_t end_scale_fixed16, AME_EasingCurve curve);

// Position Module
void AME_Position_Calculate(uint64_t elapsed_ms, uint32_t duration_ms, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, AME_EasingCurve curve, int32_t* out_x, int32_t* out_y);

// Spinner Module (Windows 11 / Linux style fluid loading spinner)
void AME_Spinner_Init(AME_Spinner* sp, int cx, int cy, int radius, int num_dots);
void AME_Spinner_SetPosition(AME_Spinner* sp, int cx, int cy);
void AME_Spinner_SetSpeed(AME_Spinner* sp, float speed_scale);
void AME_Spinner_SetAlpha(AME_Spinner* sp, uint8_t alpha);
void AME_Spinner_Update(AME_Spinner* sp, uint64_t delta_ms);
void AME_Spinner_Render(const AME_Spinner* sp, uint32_t* framebuffer, uint32_t fb_width, uint32_t fb_height, uint32_t fb_stride);

/* System Master Boot Spinner Singleton */
AME_Spinner* AME_GetBootSpinner(void);

#endif // ATOMS_AME_H
