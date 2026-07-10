#include "audio_hal.h"
#include "audio_driver_registry.h"
#include "kernel/audio/diagnostics/audio_debug.h"
#include <stddef.h>

void audio_hal_init(void) {
    audio_driver_registry_init();
    
    // In a full system, drivers would self-register here or during kernel init.
    extern void ac97_driver_register(void);
    ac97_driver_register();
    
    audio_hal_driver_t* active = audio_driver_registry_discover_active();
    if (!active) {
        // Log failure
    }
}

void audio_hal_register_driver(audio_hal_driver_t* driver) {
    audio_driver_registry_register(driver);
}

audio_hal_driver_t* audio_hal_get_active_driver(void) {
    return audio_driver_registry_discover_active();
}

// Below are generic HAL functions that forward to the active driver

bool audio_hal_start_stream(uint32_t sample_rate, uint8_t channels, uint8_t bit_depth) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->start_stream) {
        return drv->start_stream(sample_rate, channels, bit_depth);
    }
    return false;
}

void audio_hal_stop_stream(void) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->stop_stream) {
        drv->stop_stream();
    }
}

void audio_hal_pause_stream(void) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->pause_stream) {
        drv->pause_stream();
    }
}

void audio_hal_resume_stream(void) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->resume_stream) {
        drv->resume_stream();
    }
}

bool audio_hal_allocate_dma_buffer(size_t requested_size, void** virt_addr, uint32_t* phys_addr) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->allocate_dma_buffer) {
        return drv->allocate_dma_buffer(requested_size, virt_addr, phys_addr);
    }
    return false;
}

void audio_hal_update_pointers(uint32_t frames_written) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->update_pointers) {
        drv->update_pointers(frames_written);
    }
}

uint32_t audio_hal_get_hardware_position(void) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->get_hardware_position) {
        return drv->get_hardware_position();
    }
    return 0;
}

void audio_hal_set_hardware_volume(uint8_t left, uint8_t right) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->set_hardware_volume) {
        drv->set_hardware_volume(left, right);
    }
}

void audio_hal_register_irq_callback(void (*callback)(uint32_t event_flags)) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->register_irq_callback) {
        drv->register_irq_callback(callback);
    }
}

void audio_hal_shutdown(void) {
    audio_hal_driver_t* drv = audio_hal_get_active_driver();
    if (drv && drv->shutdown) {
        drv->shutdown();
    }
}
