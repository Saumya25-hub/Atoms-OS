/**
 * @file display_runtime.c
 * @brief ATOMS OS Display Intelligence Engine - Runtime Manager Implementation
 */

#include "display_runtime.h"
#include "display_geometry.h"
#include "display_layout.h"
#include "display_policy.h"

bool DIE_Runtime_OnResolutionChanged(uint32_t display_id, uint32_t new_width, uint32_t new_height) {
    DIE_DisplayInfo* info = DIE_GetDisplay(display_id);
    if (!info || !info->is_active) return false;

    if (new_width == 0 || new_height == 0) return false;
    if (info->active_mode.width == new_width && info->active_mode.height == new_height) return true;

    /* Update current active mode dimensions */
    info->active_mode.width = new_width;
    info->active_mode.height = new_height;
    info->active_mode.pitch_bytes = new_width * (info->active_mode.bpp / 8);

    /* Recalculate geometry authority */
    DIE_Geometry_Calculate(info);

    /* Push updates to kernel and AGDTE */
    DIE_Layout_UpdateSubsystems(info);

    return true;
}

void DIE_Runtime_OnDisplayHotplug(uint32_t display_id) {
    DIE_DisplayInfo* info = DIE_GetDisplay(display_id);
    if (!info) return;

    DIE_EvaluateAndApplyPolicy(display_id);
}

void DIE_Runtime_Pulse(uint64_t current_time_us) {
    DIE_DisplayInfo* info = DIE_GetPrimaryDisplay();
    if (!info || !info->is_active) return;

    /* Record runtime checkpoint timestamp */
    info->last_reconfigure_time_us = current_time_us;
}
