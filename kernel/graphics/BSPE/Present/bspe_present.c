/**
 * @file bspe_present.c
 * @brief BSPE Presentation Engine & Integration Layer (SwapFull Adapter)
 * @status Step 10 Production Implementation
 * 
 * @section PURPOSE
 * Implements the master lifecycle and presentation APIs for BSPE:
 * BSPE_Initialize, BSPE_Shutdown, BSPE_PresentFrame, and BSPE_SetSwapInterval.
 * In Step 10, BSPE becomes the official presentation entry point, while Legacy SwapFull
 * acts as the internal presentation backend to ensure 100% identical system behavior.
 * 
 * @section STRICT_COMPLIANCE
 * - Zero optimizations, zero rendering changes, zero compositor logic changes.
 * - Zero heap allocations (all subsystem handles and state are statically allocated).
 * - Screen output before and after Step 10 remains pixel-identical.
 */

#include "bspe_present.h"
#include "present_queue.h"
#include "vram_copy.h"
#include "dual_page_present.h"
#include "../Damage/damage_tracker.h"
#include "../Swapchain/swapchain.h"
#include "../FramePacer/frame_pacer.h"
#include "../Cursor/cursor_plane.h"
#include "../DisplayHAL/display_hal.h"
#include "../Drivers/vbe_driver.h"
#include "../Debug/telemetry_hud.h"
#include "kernel/debug/step14_telemetry.h"
#include <stddef.h>

/* --- Static Subsystem Handles (Zero Heap Allocation) --- */
static BSPE_PresentQueueHandle  g_bspe_queue  = NULL;
static BSPE_DamageTrackerHandle g_bspe_damage = NULL;
static BSPE_SwapchainHandle     g_bspe_swap   = NULL;
static BSPE_FramePacerHandle    g_bspe_pacer  = NULL;
static BSPE_CursorPlaneHandle   g_bspe_cursor = NULL;
static BSPE_TelemetryHUDHandle  g_bspe_hud    = NULL;

/* --- Internal Telemetry & Engine State --- */
static struct {
    uint64_t total_frames_presented;
    uint64_t total_bytes_transferred;
    uint32_t last_error_code;
    uint32_t current_swap_interval;
    bool is_initialized;
} g_engine_state = {0};

/* --- Cursor Runtime Configuration --- */
bool g_bspe_use_hardware_cursor = false;

/* --- Cursor Integration API --- */
BSPE_Error BSPE_SetCursorPosition(int32_t x, int32_t y) {
    if (!g_bspe_cursor) return BSPE_ERR_INVALID_STATE;
    
    BSPE_CursorMode current_mode;
    if (BSPE_CursorPlane_GetState(g_bspe_cursor, NULL, &current_mode, NULL, NULL, NULL, NULL) == BSPE_OK) {
        if (g_bspe_use_hardware_cursor && current_mode != BSPE_CURSOR_MODE_HARDWARE) {
            BSPE_CursorPlane_SetMode(g_bspe_cursor, BSPE_CURSOR_MODE_HARDWARE);
        } else if (!g_bspe_use_hardware_cursor && current_mode != BSPE_CURSOR_MODE_SOFTWARE) {
            BSPE_CursorPlane_SetMode(g_bspe_cursor, BSPE_CURSOR_MODE_SOFTWARE);
        }
    }
    
    g_step14_telemetry.bspe_set_position_count++;
    return BSPE_CursorPlane_SetPosition(g_bspe_cursor, x, y);
}

bool BSPE_IsHardwareCursorActive(void) {
    return BSPE_CursorPlane_IsHardwareSupported(g_bspe_cursor);
}

BSPE_Error BSPE_Initialize(const BSPE_Config* config) {
    if (!config) {
        g_engine_state.last_error_code = BSPE_ERR_NULL_POINTER;
        return BSPE_ERR_NULL_POINTER;
    }
    if (g_engine_state.is_initialized) {
        return BSPE_OK;
    }

    g_engine_state.total_frames_presented = 0;
    g_engine_state.total_bytes_transferred = 0;
    g_engine_state.last_error_code = BSPE_OK;
    g_engine_state.current_swap_interval = config->enable_vsync ? 1 : 0;

    /* Initialize Present Queue */
    BSPE_PresentQueueConfig q_cfg;
    q_cfg.capacity = 16;
    BSPE_PresentQueue_Create(&q_cfg, &g_bspe_queue);

    /* Initialize Damage Tracker */
    BSPE_DamageTrackerConfig d_cfg;
    d_cfg.screen_width = config->display_width ? config->display_width : 1024;
    d_cfg.screen_height = config->display_height ? config->display_height : 768;
    d_cfg.max_rects_per_page = 32;
    BSPE_DamageTracker_Create(&d_cfg, &g_bspe_damage);

    /* Initialize Swapchain */
    BSPE_SwapchainConfig s_cfg;
    s_cfg.buffer_count = config->buffer_count ? config->buffer_count : 2;
    s_cfg.width = d_cfg.screen_width;
    s_cfg.height = d_cfg.screen_height;
    BSPE_Swapchain_Create(&s_cfg, &g_bspe_swap);

    /* Initialize Frame Pacer */
    BSPE_FramePacerConfig p_cfg;
    p_cfg.target_refresh_rate = 60;
    p_cfg.enable_vsync = config->enable_vsync;
    BSPE_FramePacer_Create(&p_cfg, &g_bspe_pacer);

    /* Initialize Cursor Plane */
    BSPE_CursorPlaneConfig c_cfg;
    c_cfg.max_width = 64;
    c_cfg.max_height = 64;
    c_cfg.allow_software_fallback = true;
    BSPE_CursorPlane_Create(&c_cfg, &g_bspe_cursor);

    /* Initialize Telemetry HUD */
    BSPE_TelemetryHUD_Create(&g_bspe_hud);

    /* Register and initialize Display HAL drivers */
    BSPE_VBEDriver_Register();
    BSPE_DisplayHAL_InitDisplay(d_cfg.screen_width, d_cfg.screen_height, 32);

    /* Step 11: Execute Partial VRAM Copy Engine Stress Test Suite */
    BSPE_VRAM_RunStressTest();

    /* Step 12: Execute Dual-Page Damage Presentation Engine Stress Test Suite */
    BSPE_DualPage_RunStressTest();

    g_engine_state.is_initialized = true;
    return BSPE_OK;
}

void BSPE_Shutdown(void) {
    if (!g_engine_state.is_initialized) {
        return;
    }

    if (g_bspe_hud)    BSPE_TelemetryHUD_Destroy(g_bspe_hud);
    if (g_bspe_cursor) BSPE_CursorPlane_Destroy(g_bspe_cursor);
    if (g_bspe_pacer)  BSPE_FramePacer_Destroy(g_bspe_pacer);
    if (g_bspe_swap)   BSPE_Swapchain_Destroy(g_bspe_swap);
    if (g_bspe_damage) BSPE_DamageTracker_Destroy(g_bspe_damage);
    if (g_bspe_queue)  BSPE_PresentQueue_Destroy(g_bspe_queue);

    BSPE_DisplayHAL_ShutdownDisplay();

    g_bspe_hud    = NULL;
    g_bspe_cursor = NULL;
    g_bspe_pacer  = NULL;
    g_bspe_swap   = NULL;
    g_bspe_damage = NULL;
    g_bspe_queue  = NULL;
    g_engine_state.is_initialized = false;
}

void BSPE_SetSwapInterval(uint32_t swap_interval) {
    g_engine_state.current_swap_interval = swap_interval;
    if (g_bspe_pacer) {
        BSPE_FramePacer_SetRefreshRate(g_bspe_pacer, swap_interval ? 60 : 1000);
    }
}

BSPE_Error BSPE_GetTelemetryMetrics(BSPE_TelemetryMetrics* out_metrics) {
    if (!out_metrics) {
        return BSPE_ERR_NULL_POINTER;
    }
    out_metrics->fps = g_bspe_pacer ? 60 : 0;
    out_metrics->frame_time_us = 16666;
    out_metrics->vram_bytes_copied = (uint32_t)(g_engine_state.total_bytes_transferred / (g_engine_state.total_frames_presented ? g_engine_state.total_frames_presented : 1));
    out_metrics->dirty_rect_count = 1; /* Step 10 full frame copy treated as 1 dirty rect */
    out_metrics->cursor_latency_us = 0;
    out_metrics->present_queue_depth = 0;
    return BSPE_OK;
}

BSPE_Error BSPE_PresentFrame(const BOGE_StagingFrame* frame) {
    /* Auto-initialize if called prior to explicit kernel init (ensures 100% backward compatibility) */
    if (!g_engine_state.is_initialized) {
        BSPE_Config default_cfg;
        default_cfg.display_width = (frame && frame->width) ? frame->width : 1024;
        default_cfg.display_height = (frame && frame->height) ? frame->height : 768;
        default_cfg.buffer_count = 2;
        default_cfg.enable_vsync = false;
        BSPE_Initialize(&default_cfg);
    }

    /* 1. BSPE validates frame */
    if (!frame || !frame->buffer_virtual_address) {
        g_engine_state.last_error_code = BSPE_ERR_NULL_POINTER;
        return BSPE_ERR_NULL_POINTER;
    }
    if (frame->width == 0 || frame->height == 0) {
        g_engine_state.last_error_code = BSPE_ERR_INVALID_STATE;
        return BSPE_ERR_INVALID_STATE;
    }

    /* 2. BSPE submits telemetry */
    g_engine_state.total_frames_presented++;
    g_engine_state.total_bytes_transferred += (uint64_t)(frame->height * frame->pitch);

    if (g_bspe_pacer) {
        BSPE_FramePacer_BeginFrame(g_bspe_pacer, (uint64_t)g_engine_state.total_frames_presented * 16666);
    }

    if (g_bspe_hud) {
        BSPE_TelemetryMetrics metrics;
        BSPE_GetTelemetryMetrics(&metrics);
        BSPE_TelemetryHUD_UpdateMetrics(g_bspe_hud, &metrics);
    }

    /* 3. BSPE selects presentation backend & executes presentation */
    /* In Step 12, BSPE routes through BSPE_DualPage_PresentFrame() which evaluates EffectiveDamage = Union(Damage(N), Damage(N-1)) and executes partial copying or emergency fallback! */
    BSPE_DualPage_PresentFrame(g_bspe_damage, frame);

    if (g_bspe_pacer) {
        BSPE_FramePacer_EndFrame(g_bspe_pacer, ((uint64_t)g_engine_state.total_frames_presented * 16666) + 3400);
        BSPE_FramePacer_WaitForNextFrame(g_bspe_pacer);
    }

    g_engine_state.last_error_code = BSPE_OK;
    return BSPE_OK;
}
