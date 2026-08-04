#ifndef AUDIO_QUEUE_H
#define AUDIO_QUEUE_H

#include "../include/bospectra_audio_types.h"
#include "../../frame_memory/ring_buffer/circular_ring.h"
#include "../../include/bospectra_errors.h"

typedef struct {
    BOSPECTRA_CircularRing ring;
    uint32_t               underrun_count;
    uint32_t               overflow_count;
    bool                   is_initialized;
} BOSPECTRA_AudioQueue;

void              bospectra_audio_queue_init(BOSPECTRA_AudioQueue* queue);
void              bospectra_audio_queue_flush(BOSPECTRA_AudioQueue* queue);
bospectra_error_t bospectra_audio_queue_push_pcm(BOSPECTRA_AudioQueue* queue, const uint8_t* pcm_data, size_t size, size_t* out_pushed);
bospectra_error_t bospectra_audio_queue_pop_pcm(BOSPECTRA_AudioQueue* queue, uint8_t* out_pcm_data, size_t size, size_t* out_popped);
size_t            bospectra_audio_queue_get_fill_bytes(const BOSPECTRA_AudioQueue* queue);

#endif // AUDIO_QUEUE_H
