#ifndef AUDIO_CORE_H
#define AUDIO_CORE_H

#include "kernel/audio/streams/audio_stream.h"
#include <stdint.h>

typedef enum {
    AUDIO_DEVICE_OUTPUT,
    AUDIO_DEVICE_INPUT
} AudioDeviceType;

typedef struct AudioDevice {
    uint32_t device_id;
    AudioDeviceType type;
    void* driver_ops; // Pointer to future driver struct (e.g., audio_driver_ops)
    struct AudioDevice* next;
} AudioDevice;

void audio_core_init(void);
void audio_core_shutdown(void);

AudioStream* audio_core_register_stream(uint32_t process_id);
bool audio_core_destroy_stream(uint32_t stream_id);
AudioStream* audio_core_get_stream(uint32_t stream_id);
AudioStream* audio_core_get_active_streams(void);
void audio_core_destroy_streams_by_pid(uint32_t process_id);

#endif // AUDIO_CORE_H
