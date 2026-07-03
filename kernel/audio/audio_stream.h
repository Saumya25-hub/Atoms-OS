#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdbool.h>
#include "audio_buffer.h"
#include "audio_pcm.h"

typedef struct {
    uint64_t bytes_written;
    uint64_t bytes_read;
    uint64_t frames_written;
    uint64_t frames_read;
    uint32_t underrun_counter;
    uint32_t overflow_counter;
    uint32_t reset_counter;
    uint32_t flush_counter;
} AudioStreamStats;

typedef enum {
    AUDIO_STATE_CREATED,
    AUDIO_STATE_READY,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_PAUSED,
    AUDIO_STATE_STOPPED,
    AUDIO_STATE_DESTROYED
} AudioState;

typedef struct AudioStream {
    uint32_t stream_id;
    uint32_t generation_id;
    uint32_t process_id;
    AudioState state;
    uint8_t volume;
    uint8_t priority;
    AudioPcmFormat format;
    AudioStreamStats stats;
    AudioRingBuffer* ring_buffer;
    uint32_t flags;
    uint32_t ref_count;
    void* driver_handle;
    uint64_t timestamp;
    void* owner;
    
    struct AudioStream* next;
} AudioStream;

AudioStream* audio_stream_create_obj(uint32_t stream_id, uint32_t process_id);
void audio_stream_destroy_obj(AudioStream* stream);

#endif // AUDIO_STREAM_H
