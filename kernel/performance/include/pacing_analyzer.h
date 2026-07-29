/**
 * @file pacing_analyzer.h
 * @brief SignaturesOS Frame Pacing & Jitter Analyzer Engine (FPJA)
 * @status Production Kernel Performance Subsystem
 */

#ifndef BOS_PACING_ANALYZER_H
#define BOS_PACING_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>

#define FPJA_TARGET_FPS          60
#define FPJA_TARGET_FRAME_US     16666ULL /* 16.666 ms */
#define FPJA_HISTORY_SIZE        128

typedef struct {
    uint64_t total_frames;
    uint64_t last_frame_tsc;
    uint64_t last_frame_us;
    uint64_t current_frame_us;
    
    /* Pacing & Jitter Metrics */
    int64_t  pacing_error_us;    /* Difference from 16.66ms target */
    uint64_t jitter_us;          /* Frame-to-frame delta variance */
    uint64_t avg_frame_us;
    uint64_t min_frame_us;
    uint64_t max_frame_us;
    uint64_t avg_jitter_us;
    uint64_t max_jitter_us;
    uint64_t stutter_count;      /* Frames > 25ms */
    
    /* Sliding Window History */
    uint32_t history_frame_us[FPJA_HISTORY_SIZE];
    uint32_t history_jitter_us[FPJA_HISTORY_SIZE];
    uint32_t history_index;
} FPJA_State;

void FPJA_Initialize(void);
void FPJA_FrameBegin(void);
void FPJA_FrameEnd(void);
uint32_t FPJA_CalculateAdaptiveDelayUs(void);
void FPJA_GetState(FPJA_State* out_state);
void FPJA_PrintReport(void);

#endif /* BOS_PACING_ANALYZER_H */
