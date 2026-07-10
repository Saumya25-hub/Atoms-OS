#ifndef ATOMS_OS_DISPLAY_RUNTIME_H
#define ATOMS_OS_DISPLAY_RUNTIME_H

/**
 * @file display_runtime.h
 * @brief ATOMS OS Display Intelligence Engine - Runtime Manager Header
 * Coordinates dynamic resolution switches, hotplug events, rotation hooks,
 * and future multi-monitor coordination without AGDTE pipeline disruption.
 */

#include "display_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle dynamic resolution switch requests at runtime.
 */
bool DIE_Runtime_OnResolutionChanged(uint32_t display_id, uint32_t new_width, uint32_t new_height);

/**
 * @brief Handle display hotplug or virtual machine window resize event.
 */
void DIE_Runtime_OnDisplayHotplug(uint32_t display_id);

/**
 * @brief Periodic runtime pulse checking environment stability and display health.
 */
void DIE_Runtime_Pulse(uint64_t current_time_us);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_RUNTIME_H */
