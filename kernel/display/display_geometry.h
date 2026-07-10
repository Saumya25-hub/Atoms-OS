#ifndef ATOMS_OS_DISPLAY_GEOMETRY_H
#define ATOMS_OS_DISPLAY_GEOMETRY_H

/**
 * @file display_geometry.h
 * @brief ATOMS OS Display Intelligence Engine - Geometry Authority Header
 * Single authoritative calculation engine for all desktop layout rectangles:
 * desktop, wallpaper, taskbar, notification, popup, work area, cursor bounds, safe area, and dock.
 */

#include "display_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Authoritatively calculate all 9 display geometry rectangles from active mode and scale.
 */
void DIE_Geometry_Calculate(DIE_DisplayInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_GEOMETRY_H */
