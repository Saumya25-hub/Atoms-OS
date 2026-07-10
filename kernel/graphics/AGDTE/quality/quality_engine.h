#ifndef ATOMS_OS_AGDTE_QUALITY_ENGINE_H
#define ATOMS_OS_AGDTE_QUALITY_ENGINE_H

/**
 * @file quality_engine.h
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Phase 5 - Display Quality Engine Master Header
 * @status Phase 5 Quality Layer
 *
 * @section PHILOSOPHY
 * The Display Quality Engine is responsible exclusively for presentation quality, visual
 * continuity, motion fluidity, and dirty region coalescing/alignment.
 * It NEVER modifies Input Engine V2, Pointer Engine mathematics, or Cursor Engine coordinates.
 *
 * @section RULES
 * - Zero heap allocations (All data structures static/fixed-pool managed).
 * - Zero floating point arithmetic (Deterministic integer & fixed-point microsecond math).
 * - Zero blocking loops or sleeps.
 * - Single authoritative pass immediately prior to physical VRAM presentation (`AGDTE_Presenter_Execute`).
 */

#include "../include/agdte.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all Display Quality Engine modules (Stabilizer, Motion Analyzer, Dirty Optimizer, Cadence, Diag).
 * Must be called once during AGDTE_Initialize().
 */
void AGDTE_QualityEngine_Initialize(void);

/**
 * @brief Reset all Display Quality Engine internal states and history.
 */
void AGDTE_QualityEngine_Reset(void);

/**
 * @brief Process a presentation request through the Display Quality Engine prior to VRAM execution.
 * Coalesces/aligns dirty rectangles, evaluates burst pacing, regularizes cadence, and updates motion/quality telemetry.
 *
 * @param req Pointer to the presentation request to polish.
 * @param current_time_us Current system timestamp in microseconds (`timer_get_ticks() * 1000`).
 * @return AGDTE_OK on success, error code otherwise.
 */
AGDTE_Error AGDTE_QualityEngine_ProcessFrame(AGDTE_PresentRequest* req, uint64_t current_time_us);

/**
 * @brief Check if the Display Quality Engine is initialized.
 * @return true if initialized, false otherwise.
 */
bool AGDTE_QualityEngine_IsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_AGDTE_QUALITY_ENGINE_H */
