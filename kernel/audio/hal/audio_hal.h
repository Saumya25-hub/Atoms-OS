#ifndef AUDIO_HAL_H
#define AUDIO_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Audio Hardware Abstraction Layer (HAL)
 * V3 Architecture
 */

typedef struct {
    const char* name;
    uint32_t capabilities;
    
    // Lifecycle
    bool (*init)(void* device_info);
    void (*shutdown)(void);
    
    // Playback Control
    bool (*start_stream)(uint32_t sample_rate, uint8_t channels, uint8_t bit_depth);
    void (*stop_stream)(void);
    void (*pause_stream)(void);
    void (*resume_stream)(void);
    
    // Buffer Management
    bool (*allocate_dma_buffer)(size_t requested_size, void** virt_addr, uint32_t* phys_addr);
    void (*update_pointers)(uint32_t frames_written);
    uint32_t (*get_hardware_position)(void);
    
    // Routing & Volume
    void (*set_hardware_volume)(uint8_t left, uint8_t right);
    
    // Interrupts
    void (*register_irq_callback)(void (*callback)(uint32_t event_flags));
} audio_hal_driver_t;

void audio_hal_init(void);

// Global HAL functions
void audio_hal_register_driver(audio_hal_driver_t* driver);
audio_hal_driver_t* audio_hal_get_active_driver(void);

bool audio_hal_start_stream(uint32_t sample_rate, uint8_t channels, uint8_t bit_depth);
void audio_hal_stop_stream(void);
void audio_hal_pause_stream(void);
void audio_hal_resume_stream(void);
bool audio_hal_allocate_dma_buffer(size_t requested_size, void** virt_addr, uint32_t* phys_addr);
void audio_hal_update_pointers(uint32_t frames_written);
uint32_t audio_hal_get_hardware_position(void);
void audio_hal_set_hardware_volume(uint8_t left, uint8_t right);
void audio_hal_register_irq_callback(void (*callback)(uint32_t event_flags));
void audio_hal_shutdown(void);

#endif // AUDIO_HAL_H
