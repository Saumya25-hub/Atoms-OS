#ifndef ATOMS_OS_DISPLAY_CAPABILITIES_H
#define ATOMS_OS_DISPLAY_CAPABILITIES_H

/**
 * @file display_capabilities.h
 * @brief ATOMS OS Display Intelligence Engine - Capability Analysis Header
 * Collects and stores resolution lists, framebuffer metadata, stride/pitch,
 * aspect ratios, and VRAM parameters.
 */

#include "display_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Populate capability list with supported display modes and current framebuffer parameters.
 */
void DIE_Capabilities_Collect(DIE_DisplayInfo* info);

/**
 * @brief Get aspect ratio string or class for a given width x height.
 */
const char* DIE_Capabilities_GetAspectRatioString(uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_CAPABILITIES_H */
