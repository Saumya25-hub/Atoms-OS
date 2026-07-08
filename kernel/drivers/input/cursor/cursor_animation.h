#ifndef ATOMS_OS_INPUT_CURSOR_ANIMATION_H
#define ATOMS_OS_INPUT_CURSOR_ANIMATION_H

/**
 * @file cursor_animation.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Animation Scheduler
 * @section PURPOSE
 * Non-blocking timer evaluation for multi-frame animated cursors (Busy spinner, Wait hourglass).
 * Evaluates timing independently without delaying input or compositor pipelines.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Lifecycle & Initialization --- */
void cursor_animation_init(void);

/* --- Non-blocking Evaluation --- */
/**
 * @brief Evaluates current timer ticks against active sprite frame intervals.
 * @return true if the animation frame advanced and requires dirty region repaint.
 */
bool cursor_animation_tick(void);

/* --- Reset Sequence --- */
void cursor_animation_reset(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_ANIMATION_H
