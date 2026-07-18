#ifndef AUDIO_DRIVER_REGISTRY_H
#define AUDIO_DRIVER_REGISTRY_H

#include "audio_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_AUDIO_DRIVERS 8

/**
 * Audio Driver Registry
 * Responsible for discovering hardware and selecting the best driver.
 */

void audio_driver_registry_init(void);
void audio_driver_registry_register(audio_hal_driver_t* driver);
audio_hal_driver_t* audio_driver_registry_discover_active(void);

// Centralized PCI subsystem handles hardware discovery
#include "kernel/core/pci/pci.h"

#endif // AUDIO_DRIVER_REGISTRY_H
