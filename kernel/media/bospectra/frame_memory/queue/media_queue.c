#include "media_queue.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

void bospectra_media_queue_init(BOSPECTRA_MediaQueue* queue, bospectra_queue_type_t type) {
    if (!queue) return;
    memset(queue, 0, sizeof(BOSPECTRA_MediaQueue));
    queue->type = type;
    queue->is_initialized = true;
}

void bospectra_media_queue_flush(BOSPECTRA_MediaQueue* queue) {
    if (!queue || !queue->is_initialized) return;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    memset(queue->entries, 0, sizeof(queue->entries));
}

bospectra_error_t bospectra_media_queue_push(BOSPECTRA_MediaQueue* queue, void* element) {
    if (!queue || !queue->is_initialized || !element) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (queue->count >= BOSPECTRA_QUEUE_CAPACITY) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    queue->entries[queue->tail] = element;
    queue->tail = (queue->tail + 1) % BOSPECTRA_QUEUE_CAPACITY;
    queue->count++;
    queue->total_pushed++;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_media_queue_pop(BOSPECTRA_MediaQueue* queue, void** out_element) {
    if (!queue || !queue->is_initialized || !out_element) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (queue->count == 0) {
        *out_element = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    *out_element = queue->entries[queue->head];
    queue->entries[queue->head] = NULL;
    queue->head = (queue->head + 1) % BOSPECTRA_QUEUE_CAPACITY;
    queue->count--;
    queue->total_popped++;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_media_queue_peek(const BOSPECTRA_MediaQueue* queue, void** out_element) {
    if (!queue || !queue->is_initialized || !out_element) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (queue->count == 0) {
        *out_element = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    *out_element = queue->entries[queue->head];
    return BOSPECTRA_SUCCESS;
}

uint32_t bospectra_media_queue_get_count(const BOSPECTRA_MediaQueue* queue) {
    if (!queue || !queue->is_initialized) return 0;
    return queue->count;
}
