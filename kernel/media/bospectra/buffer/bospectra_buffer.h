#ifndef BOSPECTRA_BUFFER_H
#define BOSPECTRA_BUFFER_H

#include "../include/bospectra_types.h"
#include "../packet/bospectra_packet.h"

#define BOSPECTRA_RING_BUFFER_CAPACITY 64U

typedef struct {
    bospectra_buffer_id_t id;
    BOSPacket*            packets[BOSPECTRA_RING_BUFFER_CAPACITY];
    uint32_t              head;
    uint32_t              tail;
    uint32_t              count;
    size_t                total_bytes_queued;
    bool                  is_allocated;
} BOSPECTRA_RingBuffer;

void              bospectra_buffer_subsystem_init(void);
void              bospectra_buffer_subsystem_shutdown(void);
bospectra_error_t bospectra_buffer_create(bospectra_buffer_id_t* out_buffer_id);
bospectra_error_t bospectra_buffer_destroy(bospectra_buffer_id_t buffer_id);
bospectra_error_t bospectra_buffer_push(bospectra_buffer_id_t buffer_id, BOSPacket* packet);
bospectra_error_t bospectra_buffer_pop(bospectra_buffer_id_t buffer_id, BOSPacket** out_packet);
bospectra_error_t bospectra_buffer_peek(bospectra_buffer_id_t buffer_id, BOSPacket** out_packet);
uint32_t          bospectra_buffer_get_count(bospectra_buffer_id_t buffer_id);
void              bospectra_buffer_flush(bospectra_buffer_id_t buffer_id);

#endif // BOSPECTRA_BUFFER_H
