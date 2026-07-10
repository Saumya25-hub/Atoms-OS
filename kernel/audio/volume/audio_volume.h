#ifndef AUDIO_VOLUME_H
#define AUDIO_VOLUME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void audio_volume_set_master(uint8_t volume);
uint8_t audio_volume_get_master(void);

void audio_volume_set_mute(bool mute);
bool audio_volume_get_mute(void);

void audio_volume_apply_16(int16_t* buffer, size_t samples, uint8_t stream_volume, uint8_t master_volume);
void audio_volume_apply_8(int8_t* buffer, size_t samples, uint8_t stream_volume, uint8_t master_volume);
void audio_volume_apply_8u(uint8_t* buffer, size_t samples, uint8_t stream_volume, uint8_t master_volume);

#endif // AUDIO_VOLUME_H
