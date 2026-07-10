#include "kernel/audio/volume/audio_volume.h"

static uint8_t g_master_volume = 255;
static bool g_is_muted = false;

void audio_volume_set_master(uint8_t volume) {
    g_master_volume = volume;
}

uint8_t audio_volume_get_master(void) {
    return g_master_volume;
}

void audio_volume_set_mute(bool mute) {
    g_is_muted = mute;
}

bool audio_volume_get_mute(void) {
    return g_is_muted;
}

void audio_volume_apply_16(int16_t* buffer, size_t samples, uint8_t stream_volume, uint8_t master_volume) {
    if (g_is_muted || (stream_volume == 0) || (master_volume == 0)) {
        for (size_t i = 0; i < samples; i++) buffer[i] = 0;
        return;
    }
    
    if (stream_volume == 255 && master_volume == 255) return;
    
    uint32_t scale = (stream_volume * master_volume);
    
    for (size_t i = 0; i < samples; i++) {
        int32_t val = buffer[i];
        val = (val * (int32_t)scale) / (255 * 255);
        buffer[i] = (int16_t)val;
    }
}

void audio_volume_apply_8(int8_t* buffer, size_t samples, uint8_t stream_volume, uint8_t master_volume) {
    if (g_is_muted || (stream_volume == 0) || (master_volume == 0)) {
        for (size_t i = 0; i < samples; i++) buffer[i] = 0;
        return;
    }
    
    if (stream_volume == 255 && master_volume == 255) return;
    
    uint32_t scale = (stream_volume * master_volume);
    
    for (size_t i = 0; i < samples; i++) {
        int32_t val = buffer[i];
        val = (val * (int32_t)scale) / (255 * 255);
        buffer[i] = (int8_t)val;
    }
}

void audio_volume_apply_8u(uint8_t* buffer, size_t samples, uint8_t stream_volume, uint8_t master_volume) {
    if (g_is_muted || (stream_volume == 0) || (master_volume == 0)) {
        for (size_t i = 0; i < samples; i++) buffer[i] = 128;
        return;
    }
    
    if (stream_volume == 255 && master_volume == 255) return;
    
    uint32_t scale = (stream_volume * master_volume);
    
    for (size_t i = 0; i < samples; i++) {
        int32_t val = (int32_t)buffer[i] - 128;
        val = (val * (int32_t)scale) / (255 * 255);
        buffer[i] = (uint8_t)(val + 128);
    }
}
