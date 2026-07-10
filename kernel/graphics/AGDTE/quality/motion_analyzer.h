#ifndef ATOMS_OS_AGDTE_MOTION_ANALYZER_H
#define ATOMS_OS_AGDTE_MOTION_ANALYZER_H

/**
 * @file motion_analyzer.h
 * @brief AGDTE Phase 5 - Motion Continuity Analyzer Module
 * Deterministic integer analysis of cursor/window motion continuity, jitter, and frame deltas.
 */

#include "../include/agdte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Motion Analyzer module.
 */
void AGDTE_MotionAnalyzer_Initialize(void);

/**
 * @brief Reset Motion Analyzer history and variance states.
 */
void AGDTE_MotionAnalyzer_Reset(void);

/**
 * @brief Analyze motion characteristics of a presentation request (deltas, variance, consistency).
 *
 * @param req Pointer to current presentation request.
 * @param current_time_us Current system microsecond timestamp.
 */
void AGDTE_MotionAnalyzer_AnalyzeRequest(const AGDTE_PresentRequest* req, uint64_t current_time_us);

/**
 * @brief Record cursor/pointer motion coordinates (passive observation only, zero modification).
 *
 * @param x Current cursor X screen coordinate.
 * @param y Current cursor Y screen coordinate.
 * @param timestamp_us Exact observation timestamp in microseconds.
 */
void AGDTE_MotionAnalyzer_ObserveCursor(int32_t x, int32_t y, uint64_t timestamp_us);

/**
 * @brief Record window/surface motion coordinates (passive observation only).
 *
 * @param surface_id Managed surface identifier.
 * @param bounds Current window screen bounds.
 * @param timestamp_us Observation timestamp in microseconds.
 */
void AGDTE_MotionAnalyzer_ObserveSurface(uint32_t surface_id, BOGE_Rect bounds, uint64_t timestamp_us);

/**
 * @brief Get the computed presentation interval jitter in microseconds for a display.
 */
uint32_t AGDTE_MotionAnalyzer_GetJitterUs(uint32_t display_id);

/**
 * @brief Get the computed integer motion consistency score (0 to 100).
 */
uint32_t AGDTE_MotionAnalyzer_GetConsistencyScore(uint32_t display_id);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_MOTION_ANALYZER_H */
