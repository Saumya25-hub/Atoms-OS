#ifndef MEDIA_QUEUE_H
#define MEDIA_QUEUE_H

#include "../../include/bospectra_types.h"
#include "../../packet/bospectra_packet.h"
#include "../include/bospectra_frame.h"

typedef enum {
    BOSPECTRA_QUEUE_VIDEO = 0,
    BOSPECTRA_QUEUE_AUDIO,
    BOSPECTRA_QUEUE_SUBTITLE,
    BOSPECTRA_QUEUE_CONTROL,
    BOSPECTRA_QUEUE_METADATA,
    BOSPECTRA_QUEUE_TYPE_MAX
} bospectra_queue_type_t;

#define BOSPECTRA_QUEUE_CAPACITY 64U

typedef struct {
    bospectra_queue_type_t type;
    void*                  entries[BOSPECTRA_QUEUE_CAPACITY];
    uint32_t               head;
    uint32_t               tail;
    uint32_t               count;
    uint64_t               total_pushed;
    uint64_t               total_popped;
    bool                   is_initialized;
} BOSPECTRA_MediaQueue;

void              bospectra_media_queue_init(BOSPECTRA_MediaQueue* queue, bospectra_queue_type_t type);
void              bospectra_media_queue_flush(BOSPECTRA_MediaQueue* queue);
bospectra_error_t bospectra_media_queue_push(BOSPECTRA_MediaQueue* queue, void* element);
bospectra_error_t bospectra_media_queue_pop(BOSPECTRA_MediaQueue* queue, void** out_element);
bospectra_error_t bospectra_media_queue_peek(const BOSPECTRA_MediaQueue* queue, void** out_element);
uint32_t          bospectra_media_queue_get_count(const BOSPECTRA_MediaQueue* queue);

#endif // MEDIA_QUEUE_H
