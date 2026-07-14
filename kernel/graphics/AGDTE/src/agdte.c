/**
 * @file agdte.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Core Initialization & Orchestration
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Master orchestration module for the ATOMEGearDisplayTrainEngine.
 * Initializes all static subsystem tables, manages lifecycle state transitions,
 * and executes the master presentation train cadence loop (`AGDTE_Pulse`).
 */

#include "../include/agdte.h"
#include "../quality/quality_engine.h"

/* Internal reset prototypes from companion modules */
extern void agdte_display_reset_all(void);
extern void agdte_buffer_reset_all(void);
extern void agdte_surface_reset_all(void);
extern void agdte_queue_reset_all(void);
extern void agdte_backend_reset_all(void);

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern uint64_t timer_get_ticks(void);

static bool s_agdte_initialized = false;

AGDTE_Error AGDTE_Initialize(void) {
    if (s_agdte_initialized) {
        return AGDTE_ERR_INVALID_STATE;
    }

    /* Reset and initialize all static memory pools and tracking structures */
    agdte_backend_reset_all();
    agdte_display_reset_all();
    agdte_buffer_reset_all();
    agdte_surface_reset_all();
    agdte_queue_reset_all();
    AGDTE_Diag_Init();

    /* Phase 4 Timing & Synchronization Layer Initialization */
    AGDTE_Timeline_Init();
    AGDTE_Metrics_Init();
    AGDTE_RefreshController_Init();

    /* Phase 5 Display Quality Engine Initialization */
    AGDTE_QualityEngine_Initialize();

    /* Register primary VBE display entry using live screen dimensions */
    uint32_t primary_display_id = 0;
    uint32_t w = (g_kernel_screen_width > 0) ? g_kernel_screen_width : 1024;
    uint32_t h = (g_kernel_screen_height > 0) ? g_kernel_screen_height : 768;
    AGDTE_Display_Register(w, h, 32, AGDTE_BACKEND_VBE, &primary_display_id);
    AGDTE_Display_SetCadenceMode(primary_display_id, AGDTE_CADENCE_60HZ_FIXED, 60);

    /* Initialize VSync, Frame Pacer, and Swap Controller for Display 0 */
    AGDTE_VSync_Init(primary_display_id);
    AGDTE_Pacer_Init(primary_display_id, 60);
    AGDTE_SwapController_Init(primary_display_id);
    AGDTE_RefreshController_SetProfile(primary_display_id, 60, AGDTE_CADENCE_60HZ_FIXED);

    /* Pre-register master surface planes for UI hierarchy */
    uint32_t s_id;
    AGDTE_Surface_Register(primary_display_id, AGDTE_LAYER_DESKTOP, w, h, "DesktopSurface", &s_id);
    AGDTE_Surface_Register(primary_display_id, AGDTE_LAYER_WINDOWS, w, h, "WindowsSurface", &s_id);
    AGDTE_Surface_Register(primary_display_id, AGDTE_LAYER_CURSOR, 64, 64, "CursorSurface", &s_id);
    AGDTE_Surface_Register(primary_display_id, AGDTE_LAYER_POPUP, w, h, "PopupSurface", &s_id);
    AGDTE_Surface_Register(primary_display_id, AGDTE_LAYER_OVERLAY, w, h, "OverlaySurface", &s_id);
    AGDTE_Surface_Register(primary_display_id, AGDTE_LAYER_NOTIFICATION, w, h, "NotifSurface", &s_id);

    AGDTE_Diag_UpdateSurfaceCount(AGDTE_Surface_GetActiveCount());

    s_agdte_initialized = true;
    return AGDTE_OK;
}

void AGDTE_Shutdown(void) {
    if (!s_agdte_initialized) {
        return;
    }

    AGDTE_Queue_Flush();
    agdte_surface_reset_all();
    agdte_buffer_reset_all();
    agdte_display_reset_all();
    s_agdte_initialized = false;
}

bool AGDTE_IsInitialized(void) {
    return s_agdte_initialized;
}

AGDTE_Error AGDTE_Pulse(uint64_t current_time_us) {
    if (!s_agdte_initialized) {
        return AGDTE_ERR_INVALID_STATE;
    }

    uint64_t pulse_start_us = timer_get_ticks() * 1000ULL;

    /* Phase 4: Coordinate VSync polling and refresh controller timers across active displays */
    AGDTE_RefreshController_SynchronizeTimers(current_time_us);

    AGDTE_PresentRequest req;
    while (AGDTE_Queue_PeekNext(&req) == AGDTE_OK) {
        AGDTE_SchedulerDecision decision = AGDTE_Scheduler_Evaluate(&req, current_time_us);
        if (decision == AGDTE_DECISION_PRESENT_NOW || decision == AGDTE_DECISION_FORCE_PRESENT) {
            AGDTE_Queue_PopNext(&req);
            extern void inst_print_event(const char*);
            inst_print_event("AGDTE Queue Pop");
            AGDTE_Presenter_Execute(&req, current_time_us);
            AGDTE_Buffer_Unregister(req.buffer_id);
        } else if (decision == AGDTE_DECISION_SKIP_SUPERSEDED) {
            AGDTE_Queue_PopNext(&req); /* Discard cancelled request */
            AGDTE_Buffer_Unregister(req.buffer_id);
            AGDTE_Metrics_RecordSkipped();
        } else if (decision == AGDTE_DECISION_WAIT_PACING) {
            /* Top of queue is waiting for its scheduled presentation window -> yield pulse */
            break;
        } else {
            break;
        }
    }

    AGDTE_Diag_UpdateSurfaceCount(AGDTE_Surface_GetActiveCount());
    uint64_t pulse_end_us = timer_get_ticks() * 1000ULL;
    if (pulse_end_us >= pulse_start_us) {
        AGDTE_Diag_RecordPulseTime(pulse_end_us - pulse_start_us);
    }

    return AGDTE_OK;
}
