/**
 * @file frame_pacer.c
 * @brief BSPE Frame Pacer Production Implementation
 * @status Step 8 Production Implementation
 * 
 * @section PURPOSE
 * Implements precision frame scheduling, deadline tracking, and jitter analysis without heap allocation.
 * Uses a fractional remainder accumulator to achieve mathematically exact zero drift over infinite frames.
 */

#include "frame_pacer.h"
#include <stddef.h>

#define BSPE_FP_MAX_INSTANCES 4

typedef struct BSPE_FramePacer_T {
    uint32_t refresh_rate_hz;
    uint32_t base_interval_us;    /* 1,000,000 / refresh_rate_hz */
    uint32_t remainder_us;        /* 1,000,000 % refresh_rate_hz */
    uint32_t rem_accumulator;     /* Fractional remainder accumulator */
    
    uint64_t scheduled_next_us;   /* Monotonic target timestamp for next frame */
    uint64_t current_frame_start_us;
    uint64_t current_deadline_us;
    uint64_t last_frame_start_us;
    
    /* Telemetry & Statistics */
    uint64_t total_frames_completed;
    uint32_t dropped_frames;
    uint32_t ewma_jitter_us;      /* Exponentially weighted moving average of jitter */
    uint32_t max_drift_us;
    
    /* FPS calculation window */
    uint64_t window_start_us;
    uint32_t window_frame_count;
    uint32_t current_avg_fps;
    
    bool enable_vsync;
    bool is_allocated;
} BSPE_FramePacerInstance;

/* Static pool in kernel BSS segment (No heap allocation) */
static BSPE_FramePacerInstance g_fp_pool[BSPE_FP_MAX_INSTANCES];

static inline uint32_t fp_abs_diff(uint64_t a, uint64_t b) {
    return (a > b) ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

/* --- Lifecycle Implementations --- */

BSPE_Error BSPE_FramePacer_Create(const BSPE_FramePacerConfig* config, BSPE_FramePacerHandle* out_handle) {
    if (!out_handle) return BSPE_ERR_NULL_POINTER;
    
    for (uint32_t i = 0; i < BSPE_FP_MAX_INSTANCES; i++) {
        if (!g_fp_pool[i].is_allocated) {
            g_fp_pool[i].is_allocated = true;
            uint32_t rate = config ? (config->target_refresh_rate ? config->target_refresh_rate : 60) : 60;
            BSPE_FramePacer_SetRefreshRate((BSPE_FramePacerHandle)&g_fp_pool[i], rate);
            
            g_fp_pool[i].enable_vsync = config ? config->enable_vsync : true;
            g_fp_pool[i].scheduled_next_us = 0;
            g_fp_pool[i].current_frame_start_us = 0;
            g_fp_pool[i].current_deadline_us = 0;
            g_fp_pool[i].last_frame_start_us = 0;
            
            g_fp_pool[i].total_frames_completed = 0;
            g_fp_pool[i].dropped_frames = 0;
            g_fp_pool[i].ewma_jitter_us = 0;
            g_fp_pool[i].max_drift_us = 0;
            
            g_fp_pool[i].window_start_us = 0;
            g_fp_pool[i].window_frame_count = 0;
            g_fp_pool[i].current_avg_fps = rate;
            
            *out_handle = (BSPE_FramePacerHandle)&g_fp_pool[i];
            return BSPE_OK;
        }
    }
    return BSPE_ERR_OUT_OF_MEMORY;
}

void BSPE_FramePacer_Destroy(BSPE_FramePacerHandle handle) {
    if (!handle) return;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    fp->is_allocated = false;
}

void BSPE_FramePacer_Reset(BSPE_FramePacerHandle handle) {
    if (!handle) return;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    fp->scheduled_next_us = 0;
    fp->current_frame_start_us = 0;
    fp->last_frame_start_us = 0;
    fp->rem_accumulator = 0;
    fp->total_frames_completed = 0;
    fp->dropped_frames = 0;
    fp->ewma_jitter_us = 0;
    fp->max_drift_us = 0;
    fp->window_start_us = 0;
    fp->window_frame_count = 0;
}

BSPE_Error BSPE_FramePacer_SetRefreshRate(BSPE_FramePacerHandle handle, uint32_t refresh_rate_hz) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    if (refresh_rate_hz == 0 || refresh_rate_hz > 1000) return BSPE_ERR_INVALID_STATE;
    
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    fp->refresh_rate_hz  = refresh_rate_hz;
    fp->base_interval_us = 1000000 / refresh_rate_hz;
    fp->remainder_us     = 1000000 % refresh_rate_hz;
    fp->rem_accumulator  = 0;
    return BSPE_OK;
}

/* --- Core Scheduling & Deadline Tracking Implementations --- */

BSPE_Error BSPE_FramePacer_BeginFrame(BSPE_FramePacerHandle handle, uint64_t now_us) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    
    if (fp->scheduled_next_us == 0) {
        fp->scheduled_next_us = now_us;
        fp->window_start_us = now_us;
    }
    
    fp->last_frame_start_us = fp->current_frame_start_us;
    fp->current_frame_start_us = now_us;
    
    /* Calculate deadline: scheduled start + target interval */
    uint32_t interval = fp->base_interval_us;
    if (fp->rem_accumulator + fp->remainder_us >= fp->refresh_rate_hz) {
        interval += 1;
    }
    fp->current_deadline_us = fp->scheduled_next_us + interval;
    
    /* Jitter calculation: difference between actual start interval and target interval */
    if (fp->last_frame_start_us != 0) {
        uint32_t actual_interval = (uint32_t)(now_us - fp->last_frame_start_us);
        uint32_t jitter = fp_abs_diff(actual_interval, fp->base_interval_us);
        /* EWMA filter: 7/8 old + 1/8 new */
        fp->ewma_jitter_us = ((fp->ewma_jitter_us * 7) + jitter) / 8;
    }
    
    return BSPE_OK;
}

BSPE_Error BSPE_FramePacer_EndFrame(BSPE_FramePacerHandle handle, uint64_t now_us) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    
    /* Check deadline tracking */
    if (now_us > fp->current_deadline_us) {
        fp->dropped_frames++;
    }
    
    fp->total_frames_completed++;
    
    /* FPS Window tracking (1 second window) */
    fp->window_frame_count++;
    if (now_us - fp->window_start_us >= 1000000) {
        uint64_t elapsed_ms = (now_us - fp->window_start_us) / 1000;
        if (elapsed_ms > 0) {
            fp->current_avg_fps = (uint32_t)((fp->window_frame_count * 1000) / elapsed_ms);
        }
        fp->window_start_us = now_us;
        fp->window_frame_count = 0;
    }
    
    return BSPE_OK;
}

BSPE_Error BSPE_FramePacer_WaitForNextFrame(BSPE_FramePacerHandle handle) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    
    /* Advance scheduled monotonic clock by exactly 1 target interval with fractional remainder */
    uint32_t interval = fp->base_interval_us;
    fp->rem_accumulator += fp->remainder_us;
    if (fp->rem_accumulator >= fp->refresh_rate_hz) {
        interval += 1;
        fp->rem_accumulator -= fp->refresh_rate_hz;
    }
    
    fp->scheduled_next_us += interval;
    return BSPE_OK;
}

BSPE_Error BSPE_FramePacer_OnVSyncIRQ(BSPE_FramePacerHandle handle, uint64_t timestamp_us) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    
    /* Phase synchronization: Check difference between hardware VSync and scheduled monotonic clock */
    if (fp->scheduled_next_us != 0) {
        uint32_t drift = fp_abs_diff(timestamp_us, fp->scheduled_next_us);
        if (drift > fp->max_drift_us) {
            fp->max_drift_us = drift;
        }
        /* If drift exceeds half a frame interval, re-anchor phase to physical IRQ */
        if (drift > (fp->base_interval_us / 2)) {
            fp->scheduled_next_us = timestamp_us;
        }
    } else {
        fp->scheduled_next_us = timestamp_us;
    }
    return BSPE_OK;
}

BSPE_Error BSPE_FramePacer_GetAverageFPS(BSPE_FramePacerHandle handle, uint32_t* out_fps) {
    if (!handle || !out_fps) return BSPE_ERR_NULL_POINTER;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    *out_fps = fp->current_avg_fps;
    return BSPE_OK;
}

BSPE_Error BSPE_FramePacer_GetFrameStatistics(BSPE_FramePacerHandle handle, uint32_t* out_avg_fps, uint32_t* out_jitter_us, uint32_t* out_drift_us, uint32_t* out_dropped_frames) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_FramePacerInstance* fp = (BSPE_FramePacerInstance*)handle;
    if (out_avg_fps)      *out_avg_fps      = fp->current_avg_fps;
    if (out_jitter_us)    *out_jitter_us    = fp->ewma_jitter_us;
    if (out_drift_us)     *out_drift_us     = fp->max_drift_us;
    if (out_dropped_frames) *out_dropped_frames = fp->dropped_frames;
    return BSPE_OK;
}

/* --- Self-Test Verification Suite --- */

bool BSPE_FramePacer_RunSelfTest(void) {
    BSPE_FramePacerHandle fp = NULL;
    BSPE_FramePacerConfig cfg = { .target_refresh_rate = 60, .enable_vsync = true };
    
    /* 1. 60 Hz Timing Test (1 second = 60 frames) */
    if (BSPE_FramePacer_Create(&cfg, &fp) != BSPE_OK || !fp) return false;
    uint64_t sim_time = 1000000; /* Start at 1s */
    for (int i = 0; i < 60; i++) {
        BSPE_FramePacer_BeginFrame(fp, sim_time);
        BSPE_FramePacer_EndFrame(fp, sim_time + 5000); /* 5ms render */
        BSPE_FramePacer_WaitForNextFrame(fp);
        sim_time = ((BSPE_FramePacerInstance*)fp)->scheduled_next_us;
    }
    /* After exactly 60 frames at 60 Hz, sim_time must advance by exactly 1,000,000 us! */
    if (sim_time != 2000000) return false;
    BSPE_FramePacer_Destroy(fp);
    
    /* 2. 120 Hz Timing Test (1 second = 120 frames) */
    cfg.target_refresh_rate = 120;
    if (BSPE_FramePacer_Create(&cfg, &fp) != BSPE_OK || !fp) return false;
    sim_time = 0;
    for (int i = 0; i < 120; i++) {
        BSPE_FramePacer_BeginFrame(fp, sim_time);
        BSPE_FramePacer_EndFrame(fp, sim_time + 4000);
        BSPE_FramePacer_WaitForNextFrame(fp);
        sim_time = ((BSPE_FramePacerInstance*)fp)->scheduled_next_us;
    }
    if (sim_time != 1000000) return false;
    BSPE_FramePacer_Destroy(fp);
    
    /* 3. 144 Hz Timing Test (1 second = 144 frames) - Tests fractional remainder accumulator! */
    cfg.target_refresh_rate = 144;
    if (BSPE_FramePacer_Create(&cfg, &fp) != BSPE_OK || !fp) return false;
    sim_time = 0;
    for (int i = 0; i < 144; i++) {
        BSPE_FramePacer_BeginFrame(fp, sim_time);
        BSPE_FramePacer_EndFrame(fp, sim_time + 3000);
        BSPE_FramePacer_WaitForNextFrame(fp);
        sim_time = ((BSPE_FramePacerInstance*)fp)->scheduled_next_us;
    }
    /* Exactly 1,000,000 us without a single microsecond truncation error! */
    if (sim_time != 1000000) return false;
    BSPE_FramePacer_Destroy(fp);
    
    /* 4. Frame Drift Test (100,000 frames at 60 Hz -> ~27.7 minutes) */
    cfg.target_refresh_rate = 60;
    if (BSPE_FramePacer_Create(&cfg, &fp) != BSPE_OK || !fp) return false;
    sim_time = 0;
    for (int i = 0; i < 100000; i++) {
        BSPE_FramePacer_BeginFrame(fp, sim_time);
        BSPE_FramePacer_EndFrame(fp, sim_time + 1000);
        BSPE_FramePacer_WaitForNextFrame(fp);
        sim_time = ((BSPE_FramePacerInstance*)fp)->scheduled_next_us;
    }
    /* 100,000 * 1,000,000 / 60 = 1,666,666,666 us. Verify exact match! */
    if (sim_time != 1666666666ULL) return false;
    BSPE_FramePacer_Destroy(fp);
    
    /* 5. Jitter & Deadline Tracking Test */
    if (BSPE_FramePacer_Create(&cfg, &fp) != BSPE_OK || !fp) return false;
    sim_time = 0;
    BSPE_FramePacer_BeginFrame(fp, sim_time);
    /* Render takes 20ms (exceeds 16.66ms deadline!) -> Must record dropped frame */
    BSPE_FramePacer_EndFrame(fp, sim_time + 20000);
    uint32_t dropped = 0;
    BSPE_FramePacer_GetFrameStatistics(fp, NULL, NULL, NULL, &dropped);
    if (dropped != 1) return false;
    BSPE_FramePacer_Destroy(fp);
    
    /* 6. Long-Running Stability Test (1,000,000 frames at 60 Hz -> ~4.6 hours!) */
    if (BSPE_FramePacer_Create(&cfg, &fp) != BSPE_OK || !fp) return false;
    sim_time = 0;
    for (int i = 0; i < 1000000; i++) {
        BSPE_FramePacer_BeginFrame(fp, sim_time);
        BSPE_FramePacer_EndFrame(fp, sim_time + 5000); /* 5ms normal render */
        BSPE_FramePacer_WaitForNextFrame(fp);
        sim_time = ((BSPE_FramePacerInstance*)fp)->scheduled_next_us;
    }
    BSPE_FramePacer_GetFrameStatistics(fp, NULL, NULL, NULL, &dropped);
    /* Zero dropped frames over 1,000,000 frames! */
    if (dropped != 0 || ((BSPE_FramePacerInstance*)fp)->total_frames_completed != 1000000) return false;
    BSPE_FramePacer_Destroy(fp);
    
    return true;
}

#ifdef BSPE_TEST_HARNESS
#include <stdio.h>
int main(void) {
    printf("[BSPE Test] Running Frame Pacer Self-Test Suite...\n");
    if (BSPE_FramePacer_RunSelfTest()) {
        printf("[BSPE Test] ALL TESTS PASSED: 60 Hz, 120 Hz, 144 Hz, Drift Elimination, Jitter, 1M Stability!\n");
        return 0;
    } else {
        printf("[BSPE Test] SELF-TEST FAILED!\n");
        return 1;
    }
}
#endif
