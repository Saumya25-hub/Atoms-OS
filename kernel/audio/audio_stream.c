#include "audio_stream.h"
#include "audio_debug.h"
#include "../core/memory/heap/include/heap.h"

#define DEFAULT_RING_BUFFER_SIZE 262144 // 256KB — ring buffer MUST be >= DMA buffer (128KB)

static uint32_t next_generation_id = 1;

AudioStream* audio_stream_create_obj(uint32_t stream_id, uint32_t process_id) {
    AudioStream* stream = (AudioStream*)kmalloc(sizeof(AudioStream));
    if (!stream) {
        audio_debug_log_fail_alloc();
        return NULL;
    }

    stream->ring_buffer = audio_buffer_create(DEFAULT_RING_BUFFER_SIZE);
    if (!stream->ring_buffer) {
        kfree(stream);
        audio_debug_log_fail_alloc();
        return NULL;
    }

    stream->stream_id = stream_id;
    stream->generation_id = next_generation_id++;
    stream->process_id = process_id;
    stream->state = AUDIO_STATE_CREATED;
    stream->volume = 255;
    stream->priority = 128;
    stream->format.format = PCM_FORMAT_S16_LE;
    stream->format.sample_rate = 44100;
    stream->format.channels = 2;
    stream->format.bit_depth = 16;
    stream->format.is_signed = true;

    stream->stats.bytes_written = 0;
    stream->stats.bytes_read = 0;
    stream->stats.frames_written = 0;
    stream->stats.frames_read = 0;
    stream->stats.underrun_counter = 0;
    stream->stats.overflow_counter = 0;
    stream->stats.reset_counter = 0;
    stream->stats.flush_counter = 0;
    stream->flags = 0;
    stream->ref_count = 1;
    stream->driver_handle = NULL;
    stream->timestamp = 0;
    stream->owner = NULL;
    stream->next = NULL;

    audio_debug_log_alloc(sizeof(AudioStream));
    audio_debug_log_stream_create();

    return stream;
}

void audio_stream_destroy_obj(AudioStream* stream) {
    if (!stream) return;

    // Destroy the exclusively owned ring buffer
    if (stream->ring_buffer) {
        audio_buffer_destroy(stream->ring_buffer);
        stream->ring_buffer = NULL;
    }

    kfree(stream);
    audio_debug_log_free(sizeof(AudioStream));
    audio_debug_log_stream_destroy();
}
