#ifndef AUDIO_BRIDGE_H
#define AUDIO_BRIDGE_H

#include "../include/bospectra_audio_types.h"
#include "../../include/bospectra_errors.h"

// Audio Bridge Lifecycle & Subsystem Linking
bospectra_error_t bospectra_audio_bridge_init(void);
void              bospectra_audio_bridge_shutdown(void);

// Transmit PCM Audio Buffer to OS Audio HAL / ac97_playback Subsystem
bospectra_error_t bospectra_audio_bridge_write_pcm(const uint8_t* pcm_data, size_t size_bytes, const BOSPECTRA_AudioSpec* spec);

// Hardware Driver Connection Status Query
bool bospectra_audio_bridge_is_hardware_ready(void);

#endif // AUDIO_BRIDGE_H
