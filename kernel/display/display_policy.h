#ifndef ATOMS_OS_DISPLAY_POLICY_H
#define ATOMS_OS_DISPLAY_POLICY_H

/**
 * @file display_policy.h
 * @brief ATOMS OS Display Intelligence Engine - Policy Engine Header
 * Evaluates available modes against detected environment, visible area, aspect ratio,
 * hypervisor window constraints, and performance targets without static priorities.
 */

#include "display_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Evaluate all candidate modes and select the optimal resolution for the current environment.
 */
bool DIE_Policy_EvaluateBestMode(DIE_DisplayInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_POLICY_H */
