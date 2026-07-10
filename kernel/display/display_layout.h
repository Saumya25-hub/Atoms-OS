#ifndef ATOMS_OS_DISPLAY_LAYOUT_H
#define ATOMS_OS_DISPLAY_LAYOUT_H

/**
 * @file display_layout.h
 * @brief ATOMS OS Display Intelligence Engine - Layout & Subsystem Synchronization Header
 * Acts as the authoritative synchronization bridge, pushing geometry calculations into
 * kernel globals, AGDTE display registration, BWE compositor clipping, and pointer bounds.
 */

#include "display_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Authoritatively push computed geometry into all kernel, WM, and AGDTE subsystems.
 */
void DIE_Layout_UpdateSubsystems(DIE_DisplayInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_LAYOUT_H */
