#include "audio_queue.h"
#include "kernel/core/lib/include/string.h"

void bospectra_audio_queue_init(BOSPECTRA_AudioQueue* queue) {
    if (!queue) return;
    memset(queue, 0, sizeof(BOSPECTRA_AudioQueue));
    bospectra_circular_ring_init(&queue->ring);
    queue->is_initialized = true;
}

void bospectra_audio_queue_flush(BOSPECTRA_AudioQueue* queue) {
    if (!queue || !queue->is_initialized) return;
    bospectra_circular_ring_flush(&queue->ring);
}

size_t bospectra_audio_queue_get_fill_bytes(const BOSPECTRA_AudioQueue* queue) {
    if (!queue || !queue->is_initialized) return 0;
    return bospectra_circular_ring_get_used_space(&queue->ring);
}

bospectra_error_t bospectra_audio_queue_push_pcm(BOSPECTRA_AudioQueue* queue, const uint8_t* pcm_data, size_t size, size_t* out_pushed) {
    if (!queue || !queue->is_initialized || !pcm_data) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_error_t err = bospectra_circular_ring_write(&queue->ring, pcm_data, size, out_pushed);
    if (err == BOSPECTRA_ERR_BUFFER_OVERFLOW) {
        queue->overflow_count++;
    }
    return err;
}

bospectra_error_t bospectra_audio_queue_pop_pcm(BOSPECTRA_AudioQueue* queue, uint8_t* out_pcm_data, size_t size, size_t* out_popped) {
    if (!queue || !queue->is_initialized || !out_pcm_data) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_error_t err = bospectra_circular_ring_read(&queue->ring, out_pcm_data, size, out_popped);
    if (err == BOSPECTRA_ERR_BUFFER_UNDERFLOW) {
        queue->underrun_count++;
    }
    return err;
}
