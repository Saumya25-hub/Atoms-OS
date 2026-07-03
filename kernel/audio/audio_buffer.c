#include "audio_buffer.h"
#include "audio_debug.h"
#include "../core/memory/heap/include/heap.h"
#include "../drivers/display/display.h"

AudioRingBuffer* audio_buffer_create(size_t size) {
    if (size == 0) return NULL;

    AudioRingBuffer* buffer = (AudioRingBuffer*)kmalloc(sizeof(AudioRingBuffer));
    if (!buffer) {
        audio_debug_log_fail_alloc();
        return NULL;
    }

    buffer->data = (uint8_t*)kmalloc(size);
    if (!buffer->data) {
        kfree(buffer);
        audio_debug_log_fail_alloc();
        return NULL;
    }

    buffer->capacity = size;
    buffer->head = 0;
    buffer->tail = 0;

    audio_debug_log_alloc(sizeof(AudioRingBuffer) + size);
    return buffer;
}

void audio_buffer_destroy(AudioRingBuffer* buffer) {
    if (!buffer) return;

    if (buffer->data) {
        kfree(buffer->data);
    }
    
    uint32_t total_freed = sizeof(AudioRingBuffer) + buffer->capacity;
    kfree(buffer);
    
    audio_debug_log_free(total_freed);
}

void audio_buffer_reset(AudioRingBuffer* buffer) {
    if (!buffer) return;
    buffer->head = 0;
    buffer->tail = 0;
}

size_t audio_buffer_available(AudioRingBuffer* buffer) {
    if (!buffer) return 0;
    if (buffer->head >= buffer->tail) {
        return buffer->head - buffer->tail;
    } else {
        return buffer->capacity - (buffer->tail - buffer->head);
    }
}

size_t audio_buffer_free_space(AudioRingBuffer* buffer) {
    if (!buffer) return 0;
    // We keep 1 byte free to distinguish empty from full.
    size_t available = audio_buffer_available(buffer);
    return buffer->capacity - available - 1;
}

bool audio_buffer_empty(AudioRingBuffer* buffer) {
    if (!buffer) return true;
    return buffer->head == buffer->tail;
}

bool audio_buffer_full(AudioRingBuffer* buffer) {
    if (!buffer) return true;
    return audio_buffer_free_space(buffer) == 0;
}

size_t audio_buffer_write(AudioRingBuffer* buffer, const uint8_t* data, size_t size) {
    if (!buffer || !data || size == 0) return 0;

    size_t free_space = audio_buffer_free_space(buffer);
    if (size > free_space) {
        size = free_space;
    }

    if (size == 0) return 0;

    size_t space_until_end = buffer->capacity - buffer->head;
    if (size <= space_until_end) {
        // Simple copy
        for (size_t i = 0; i < size; i++) {
            buffer->data[buffer->head + i] = data[i];
        }
    } else {
        // Wrap-around copy
        for (size_t i = 0; i < space_until_end; i++) {
            buffer->data[buffer->head + i] = data[i];
        }
        for (size_t i = 0; i < size - space_until_end; i++) {
            buffer->data[i] = data[space_until_end + i];
        }
    }

    buffer->head = (buffer->head + size) % buffer->capacity;
    return size;
}

size_t audio_buffer_read(AudioRingBuffer* buffer, uint8_t* data, size_t size) {
    if (!buffer || !data || size == 0) return 0;

    size_t available = audio_buffer_available(buffer);
    if (size > available) {
        size = available;
    }

    if (size == 0) return 0;

    size_t data_until_end = buffer->capacity - buffer->tail;
    if (size <= data_until_end) {
        // Simple copy
        for (size_t i = 0; i < size; i++) {
            data[i] = buffer->data[buffer->tail + i];
        }
    } else {
        // Wrap-around copy
        for (size_t i = 0; i < data_until_end; i++) {
            data[i] = buffer->data[buffer->tail + i];
        }
        for (size_t i = 0; i < size - data_until_end; i++) {
            data[data_until_end + i] = buffer->data[i];
        }
    }

    buffer->tail = (buffer->tail + size) % buffer->capacity;
    return size;
}
