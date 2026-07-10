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

// Exposing basic PCI reading for drivers if they need it during init
uint32_t audio_pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void audio_pci_write_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);

#endif // AUDIO_DRIVER_REGISTRY_H
