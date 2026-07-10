#ifndef ATOMS_OS_DISPLAY_DETECTION_H
#define ATOMS_OS_DISPLAY_DETECTION_H

/**
 * @file display_detection.h
 * @brief ATOMS OS Display Intelligence Engine - Hardware Detection Header
 * Probes PCI bus and VBE signatures to identify Virtual Machines, QEMU, VirtualBox, VMware,
 * Hyper-V, and Physical GPU hardware without hardcoded workarounds.
 */

#include "display_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detect runtime environment and hardware display controller capabilities.
 * @param out_env Output detected environment type.
 * @param out_env_name Output environment string buffer (min 32 bytes).
 * @param out_ctrl_name Output controller string buffer (min 64 bytes).
 */
void DIE_Detection_DetectEnvironment(DIE_EnvironmentType* out_env, char* out_env_name, char* out_ctrl_name);

/**
 * @brief Query physical VRAM capacity from display hardware or VBE controller.
 */
uint32_t DIE_Detection_GetVRAMSize(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_OS_DISPLAY_DETECTION_H */
