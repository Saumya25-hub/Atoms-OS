#ifndef CIRCULAR_RING_H
#define CIRCULAR_RING_H

#include "../../include/bospectra_types.h"

#define BOSPECTRA_CIRCULAR_RING_SIZE (1024U * 64U) // 64 KB Ring Buffer

typedef struct {
    uint8_t  buffer[BOSPECTRA_CIRCULAR_RING_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t fill_level;
    uint64_t total_bytes_written;
    uint64_t total_bytes_read;
    bool     is_initialized;
} BOSPECTRA_CircularRing;

void              bospectra_circular_ring_init(BOSPECTRA_CircularRing* ring);
void              bospectra_circular_ring_flush(BOSPECTRA_CircularRing* ring);
bospectra_error_t bospectra_circular_ring_write(BOSPECTRA_CircularRing* ring, const uint8_t* data, size_t size, size_t* out_written);
bospectra_error_t bospectra_circular_ring_read(BOSPECTRA_CircularRing* ring, uint8_t* out_data, size_t size, size_t* out_read);
bospectra_error_t bospectra_circular_ring_peek(const BOSPECTRA_CircularRing* ring, uint8_t* out_data, size_t size, size_t* out_peeked);
size_t            bospectra_circular_ring_get_free_space(const BOSPECTRA_CircularRing* ring);
size_t            bospectra_circular_ring_get_used_space(const BOSPECTRA_CircularRing* ring);

#endif // CIRCULAR_RING_H
