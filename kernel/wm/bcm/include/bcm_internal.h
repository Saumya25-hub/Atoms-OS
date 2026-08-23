#ifndef BCM_INTERNAL_H
#define BCM_INTERNAL_H

#include "bcm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* BCM Internal Configuration Constants                                      */
/* ========================================================================= */
#define BCM_DEFAULT_PACING_INTERVAL_MS 16U  /* ~60 FPS nominal (~16.666 ms) */
#define BCM_MIN_INTER_FRAME_GAP_MS    10U  /* Max 100 FPS under rapid continuous input */
#define BCM_MAX_DAMAGE_QUEUE_CAPACITY 64U
#define BCM_SCREEN_COLLAPSE_PERCENT   65U  /* >65% dirty area collapses to full screen */

/* ========================================================================= */
/* BCM Core Internal State Structure (Static Preallocated Envelope)          */
/* ========================================================================= */
typedef struct {
    bool initialized;
    volatile BCM_FrameState state;
    volatile bool is_composing;
    volatile bool is_presenting;
    volatile bool pending_damage;
    volatile bool full_damage_requested;
    
    /* Active Dirty Bounding Boxes (Static Envelope) */
    BCM_Rect dirty_rects[BCM_MAX_DIRTY_RECTS];
    uint32_t dirty_count;

    /* Pacing & Timing Counters */
    uint64_t last_timer_tick;
    uint64_t last_compose_tick;
    uint64_t last_present_tick;
    uint32_t pacing_interval_ms;
    uint32_t min_inter_frame_ms;

    /* Rolling FPS calculation window */
    uint64_t fps_window_start_tick;
    uint32_t fps_window_frame_count;

    /* Performance & Reliability Counters */
    BCM_Telemetry telemetry;
} BCM_CoreState;

/* Global internal instance in BSS */
extern BCM_CoreState g_bcm_state;

/* Internal damage and coalescing helpers */
void BCM_Internal_CoalesceDamage(void);
void BCM_Internal_ResetDirtyRects(void);

#ifdef __cplusplus
}
#endif

#endif /* BCM_INTERNAL_H */
