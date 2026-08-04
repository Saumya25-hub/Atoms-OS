#include "circular_ring.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

void bospectra_circular_ring_init(BOSPECTRA_CircularRing* ring) {
    if (!ring) return;
    memset(ring, 0, sizeof(BOSPECTRA_CircularRing));
    ring->is_initialized = true;
}

void bospectra_circular_ring_flush(BOSPECTRA_CircularRing* ring) {
    if (!ring || !ring->is_initialized) return;
    ring->head = 0;
    ring->tail = 0;
    ring->fill_level = 0;
}

size_t bospectra_circular_ring_get_free_space(const BOSPECTRA_CircularRing* ring) {
    if (!ring || !ring->is_initialized) return 0;
    return BOSPECTRA_CIRCULAR_RING_SIZE - ring->fill_level;
}

size_t bospectra_circular_ring_get_used_space(const BOSPECTRA_CircularRing* ring) {
    if (!ring || !ring->is_initialized) return 0;
    return ring->fill_level;
}

bospectra_error_t bospectra_circular_ring_write(BOSPECTRA_CircularRing* ring, const uint8_t* data, size_t size, size_t* out_written) {
    if (!ring || !ring->is_initialized || !data) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (size == 0) {
        if (out_written) *out_written = 0;
        return BOSPECTRA_SUCCESS;
    }

    size_t free_space = bospectra_circular_ring_get_free_space(ring);
    if (free_space == 0) {
        if (out_written) *out_written = 0;
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    size_t bytes_to_write = (size > free_space) ? free_space : size;
    size_t first_chunk = BOSPECTRA_CIRCULAR_RING_SIZE - ring->tail;

    if (bytes_to_write <= first_chunk) {
        memcpy(&ring->buffer[ring->tail], data, bytes_to_write);
        ring->tail = (ring->tail + bytes_to_write) % BOSPECTRA_CIRCULAR_RING_SIZE;
    } else {
        memcpy(&ring->buffer[ring->tail], data, first_chunk);
        memcpy(&ring->buffer[0], data + first_chunk, bytes_to_write - first_chunk);
        ring->tail = bytes_to_write - first_chunk;
    }

    ring->fill_level += bytes_to_write;
    ring->total_bytes_written += bytes_to_write;
    if (out_written) *out_written = bytes_to_write;

    return (bytes_to_write < size) ? BOSPECTRA_ERR_BUFFER_OVERFLOW : BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_circular_ring_read(BOSPECTRA_CircularRing* ring, uint8_t* out_data, size_t size, size_t* out_read) {
    if (!ring || !ring->is_initialized || !out_data) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (size == 0) {
        if (out_read) *out_read = 0;
        return BOSPECTRA_SUCCESS;
    }

    size_t used_space = bospectra_circular_ring_get_used_space(ring);
    if (used_space == 0) {
        if (out_read) *out_read = 0;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    size_t bytes_to_read = (size > used_space) ? used_space : size;
    size_t first_chunk = BOSPECTRA_CIRCULAR_RING_SIZE - ring->head;

    if (bytes_to_read <= first_chunk) {
        memcpy(out_data, &ring->buffer[ring->head], bytes_to_read);
        ring->head = (ring->head + bytes_to_read) % BOSPECTRA_CIRCULAR_RING_SIZE;
    } else {
        memcpy(out_data, &ring->buffer[ring->head], first_chunk);
        memcpy(out_data + first_chunk, &ring->buffer[0], bytes_to_read - first_chunk);
        ring->head = bytes_to_read - first_chunk;
    }

    ring->fill_level -= bytes_to_read;
    ring->total_bytes_read += bytes_to_read;
    if (out_read) *out_read = bytes_to_read;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_circular_ring_peek(const BOSPECTRA_CircularRing* ring, uint8_t* out_data, size_t size, size_t* out_peeked) {
    if (!ring || !ring->is_initialized || !out_data) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    size_t used_space = bospectra_circular_ring_get_used_space(ring);
    if (used_space == 0) {
        if (out_peeked) *out_peeked = 0;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    size_t bytes_to_peek = (size > used_space) ? used_space : size;
    size_t first_chunk = BOSPECTRA_CIRCULAR_RING_SIZE - ring->head;

    if (bytes_to_peek <= first_chunk) {
        memcpy(out_data, &ring->buffer[ring->head], bytes_to_peek);
    } else {
        memcpy(out_data, &ring->buffer[ring->head], first_chunk);
        memcpy(out_data + first_chunk, &ring->buffer[0], bytes_to_peek - first_chunk);
    }

    if (out_peeked) *out_peeked = bytes_to_peek;
    return BOSPECTRA_SUCCESS;
}
