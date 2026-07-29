/**
 * @file pacing_analyzer.c
 * @brief SignaturesOS Frame Pacing & Jitter Analyzer Engine Implementation
 */

#include "kernel/performance/include/pacing_analyzer.h"
#include <stddef.h>

extern uint64_t timer_get_ticks(void);
extern void display_print(const char* s);
extern void display_print_dec(uint64_t val);

static FPJA_State g_fpja_state;
static bool       g_fpja_initialized = false;
static uint64_t   s_frame_start_ticks = 0;
static uint64_t   s_last_frame_ticks = 0;

static inline uint64_t rdtsc_pure(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void FPJA_Initialize(void) {
    g_fpja_state.total_frames = 0;
    g_fpja_state.last_frame_tsc = 0;
    g_fpja_state.last_frame_us = FPJA_TARGET_FRAME_US;
    g_fpja_state.current_frame_us = FPJA_TARGET_FRAME_US;
    g_fpja_state.pacing_error_us = 0;
    g_fpja_state.jitter_us = 0;
    g_fpja_state.avg_frame_us = FPJA_TARGET_FRAME_US;
    g_fpja_state.min_frame_us = 0xFFFFFFFF;
    g_fpja_state.max_frame_us = 0;
    g_fpja_state.avg_jitter_us = 0;
    g_fpja_state.max_jitter_us = 0;
    g_fpja_state.stutter_count = 0;
    g_fpja_state.history_index = 0;

    for (int i = 0; i < FPJA_HISTORY_SIZE; i++) {
        g_fpja_state.history_frame_us[i] = (uint32_t)FPJA_TARGET_FRAME_US;
        g_fpja_state.history_jitter_us[i] = 0;
    }

    g_fpja_initialized = true;
    s_last_frame_ticks = timer_get_ticks();
}

void FPJA_FrameBegin(void) {
    if (!g_fpja_initialized) FPJA_Initialize();
    s_frame_start_ticks = timer_get_ticks();
}

void FPJA_FrameEnd(void) {
    if (!g_fpja_initialized) return;

    uint64_t now_ticks = timer_get_ticks();
    uint64_t frame_delta_ms = now_ticks - s_last_frame_ticks;
    s_last_frame_ticks = now_ticks;

    uint64_t frame_delta_us = frame_delta_ms * 1000ULL;
    if (frame_delta_us == 0) frame_delta_us = FPJA_TARGET_FRAME_US;

    g_fpja_state.total_frames++;
    g_fpja_state.current_frame_us = frame_delta_us;

    /* Calculate Jitter (difference from previous frame time) */
    uint64_t prev_us = g_fpja_state.last_frame_us;
    uint64_t jitter = (frame_delta_us > prev_us) ? (frame_delta_us - prev_us) : (prev_us - frame_delta_us);
    g_fpja_state.jitter_us = jitter;
    g_fpja_state.last_frame_us = frame_delta_us;

    /* Pacing Error from 16.66ms Target */
    g_fpja_state.pacing_error_us = (int64_t)frame_delta_us - (int64_t)FPJA_TARGET_FRAME_US;

    /* Min / Max Tracking */
    if (frame_delta_us < g_fpja_state.min_frame_us) g_fpja_state.min_frame_us = frame_delta_us;
    if (frame_delta_us > g_fpja_state.max_frame_us) g_fpja_state.max_frame_us = frame_delta_us;
    if (jitter > g_fpja_state.max_jitter_us) g_fpja_state.max_jitter_us = jitter;

    if (frame_delta_us > 25000ULL) { /* Frame > 25ms = Stutter Event */
        g_fpja_state.stutter_count++;
    }

    /* Record in Sliding Window */
    uint32_t idx = g_fpja_state.history_index % FPJA_HISTORY_SIZE;
    g_fpja_state.history_frame_us[idx] = (uint32_t)frame_delta_us;
    g_fpja_state.history_jitter_us[idx] = (uint32_t)jitter;
    g_fpja_state.history_index++;

    /* Compute Rolling Averages */
    uint64_t sum_frame = 0;
    uint64_t sum_jitter = 0;
    uint32_t count = (g_fpja_state.total_frames < FPJA_HISTORY_SIZE) ? (uint32_t)g_fpja_state.total_frames : FPJA_HISTORY_SIZE;
    if (count == 0) count = 1;

    for (uint32_t i = 0; i < count; i++) {
        sum_frame += g_fpja_state.history_frame_us[i];
        sum_jitter += g_fpja_state.history_jitter_us[i];
    }
    g_fpja_state.avg_frame_us = sum_frame / count;
    g_fpja_state.avg_jitter_us = sum_jitter / count;
}

uint32_t FPJA_CalculateAdaptiveDelayUs(void) {
    if (!g_fpja_initialized) return 0;
    uint64_t elapsed_ms = timer_get_ticks() - s_frame_start_ticks;
    uint64_t elapsed_us = elapsed_ms * 1000ULL;

    if (elapsed_us < FPJA_TARGET_FRAME_US) {
        return (uint32_t)(FPJA_TARGET_FRAME_US - elapsed_us);
    }
    return 0;
}

void FPJA_GetState(FPJA_State* out_state) {
    if (out_state) {
        *out_state = g_fpja_state;
    }
}

void FPJA_PrintReport(void) {
    display_print("\n========================================\n");
    display_print("  FPJA FRAME PACING & JITTER REPORT      \n");
    display_print("========================================\n");
    display_print("Total Frames   : "); display_print_dec(g_fpja_state.total_frames); display_print("\n");
    display_print("Target Cadence : 16666 us (60.0 FPS)\n");
    display_print("Average Frame  : "); display_print_dec(g_fpja_state.avg_frame_us); display_print(" us\n");
    display_print("Average Jitter : "); display_print_dec(g_fpja_state.avg_jitter_us); display_print(" us\n");
    display_print("Max Jitter     : "); display_print_dec(g_fpja_state.max_jitter_us); display_print(" us\n");
    display_print("Stutter Events : "); display_print_dec(g_fpja_state.stutter_count); display_print("\n");
    display_print("Pacing Status  : ");
    if (g_fpja_state.avg_jitter_us < 1000) {
        display_print("ULTRA-STABLE (60 FPS VSYNC ALIGNED)\n");
    } else if (g_fpja_state.avg_jitter_us < 5000) {
        display_print("GOOD (MINOR EMULATION VARIANCE)\n");
    } else {
        display_print("JITTER DETECTED (HIGH VARIANCE)\n");
    }
    display_print("========================================\n");
}
