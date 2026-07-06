#ifndef ATOMS_OS_BSPE_VBE_DRIVER_H
#define ATOMS_OS_BSPE_VBE_DRIVER_H

/**
 * @file vbe_driver.h
 * @brief BSPE Bochs VBE / VESA Hardware Driver Backend Public Header
 * @status Step 4 Production Implementation
 * 
 * @section PURPOSE
 * Defines the public registration API and instance accessors for the Bochs VBE / VESA
 * Linear Framebuffer (LFB) physical hardware driver backend.
 * 
 * @section DEPENDENCY_LIST
 * - "../include/bspe.h"
 * - "../DisplayHAL/display_hal.h"
 * - Zero circular includes.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"
#include "../DisplayHAL/display_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the Bochs VBE / VESA driver backend with BSPE Display HAL and sets it active.
 * @return true on successful registration; false on failure.
 */
bool BSPE_VBEDriver_Register(void);

/**
 * @brief Retrieves the active driver handle for the registered VBE driver instance.
 */
BSPE_DisplayDriverHandle BSPE_VBEDriver_GetInstance(void);

/**
 * @brief Retrieves the immutable hardware driver interface table for VBE.
 */
const BSPE_DisplayDriverInterface* BSPE_VBEDriver_GetInterface(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_VBE_DRIVER_H
