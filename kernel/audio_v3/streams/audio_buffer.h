#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint8_t* data;
    size_t capacity;
    volatile size_t head;
    volatile size_t tail;
} AudioRingBuffer;

AudioRingBuffer* audio_buffer_create(size_t size);
void audio_buffer_destroy(AudioRingBuffer* buffer);
void audio_buffer_reset(AudioRingBuffer* buffer);

size_t audio_buffer_write(AudioRingBuffer* buffer, const uint8_t* data, size_t size);
size_t audio_buffer_read(AudioRingBuffer* buffer, uint8_t* data, size_t size);

size_t audio_buffer_available(AudioRingBuffer* buffer);
size_t audio_buffer_free_space(AudioRingBuffer* buffer);
bool audio_buffer_empty(AudioRingBuffer* buffer);
bool audio_buffer_full(AudioRingBuffer* buffer);

#endif // AUDIO_BUFFER_H
