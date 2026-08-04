#include "bospectra_buffer.h"
#include "../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_RING_BUFFERS 8U

static BOSPECTRA_RingBuffer g_bospectra_buffers[BOSPECTRA_MAX_RING_BUFFERS];
static bool g_bospectra_buffer_initialized = false;

void bospectra_buffer_subsystem_init(void) {
    memset(g_bospectra_buffers, 0, sizeof(g_bospectra_buffers));
    g_bospectra_buffer_initialized = true;
}

void bospectra_buffer_subsystem_shutdown(void) {
    for (uint32_t i = 0; i < BOSPECTRA_MAX_RING_BUFFERS; i++) {
        if (g_bospectra_buffers[i].is_allocated) {
            bospectra_buffer_destroy(g_bospectra_buffers[i].id);
        }
    }
    memset(g_bospectra_buffers, 0, sizeof(g_bospectra_buffers));
    g_bospectra_buffer_initialized = false;
}

bospectra_error_t bospectra_buffer_create(bospectra_buffer_id_t* out_buffer_id) {
    if (!g_bospectra_buffer_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_buffer_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_MAX_RING_BUFFERS; i++) {
        if (!g_bospectra_buffers[i].is_allocated) {
            memset(&g_bospectra_buffers[i], 0, sizeof(BOSPECTRA_RingBuffer));
            g_bospectra_buffers[i].id = i + 1;
            g_bospectra_buffers[i].is_allocated = true;
            *out_buffer_id = g_bospectra_buffers[i].id;
            return BOSPECTRA_SUCCESS;
        }
    }

    return BOSPECTRA_ERR_OUT_OF_MEMORY;
}

void bospectra_buffer_flush(bospectra_buffer_id_t buffer_id) {
    if (buffer_id == 0 || buffer_id > BOSPECTRA_MAX_RING_BUFFERS) return;

    uint32_t idx = buffer_id - 1;
    if (!g_bospectra_buffers[idx].is_allocated) return;

    while (g_bospectra_buffers[idx].count > 0) {
        BOSPacket* pkt = g_bospectra_buffers[idx].packets[g_bospectra_buffers[idx].head];
        if (pkt) {
            bospectra_packet_free(pkt);
        }
        g_bospectra_buffers[idx].head = (g_bospectra_buffers[idx].head + 1) % BOSPECTRA_RING_BUFFER_CAPACITY;
        g_bospectra_buffers[idx].count--;
    }
    g_bospectra_buffers[idx].head = 0;
    g_bospectra_buffers[idx].tail = 0;
    g_bospectra_buffers[idx].total_bytes_queued = 0;
}

bospectra_error_t bospectra_buffer_destroy(bospectra_buffer_id_t buffer_id) {
    if (!g_bospectra_buffer_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (buffer_id == 0 || buffer_id > BOSPECTRA_MAX_RING_BUFFERS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = buffer_id - 1;
    if (!g_bospectra_buffers[idx].is_allocated) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_buffer_flush(buffer_id);
    memset(&g_bospectra_buffers[idx], 0, sizeof(BOSPECTRA_RingBuffer));
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_buffer_push(bospectra_buffer_id_t buffer_id, BOSPacket* packet) {
    if (!g_bospectra_buffer_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!packet) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (buffer_id == 0 || buffer_id > BOSPECTRA_MAX_RING_BUFFERS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = buffer_id - 1;
    if (!g_bospectra_buffers[idx].is_allocated) return BOSPECTRA_ERR_HANDLE_INVALID;
    if (g_bospectra_buffers[idx].count >= BOSPECTRA_RING_BUFFER_CAPACITY) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    g_bospectra_buffers[idx].packets[g_bospectra_buffers[idx].tail] = packet;
    g_bospectra_buffers[idx].tail = (g_bospectra_buffers[idx].tail + 1) % BOSPECTRA_RING_BUFFER_CAPACITY;
    g_bospectra_buffers[idx].count++;
    g_bospectra_buffers[idx].total_bytes_queued += packet->size;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_buffer_pop(bospectra_buffer_id_t buffer_id, BOSPacket** out_packet) {
    if (!g_bospectra_buffer_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_packet) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (buffer_id == 0 || buffer_id > BOSPECTRA_MAX_RING_BUFFERS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = buffer_id - 1;
    if (!g_bospectra_buffers[idx].is_allocated) return BOSPECTRA_ERR_HANDLE_INVALID;
    if (g_bospectra_buffers[idx].count == 0) {
        *out_packet = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    *out_packet = g_bospectra_buffers[idx].packets[g_bospectra_buffers[idx].head];
    g_bospectra_buffers[idx].packets[g_bospectra_buffers[idx].head] = NULL;
    g_bospectra_buffers[idx].head = (g_bospectra_buffers[idx].head + 1) % BOSPECTRA_RING_BUFFER_CAPACITY;
    g_bospectra_buffers[idx].count--;
    if (*out_packet) {
        g_bospectra_buffers[idx].total_bytes_queued -= (*out_packet)->size;
    }

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_buffer_peek(bospectra_buffer_id_t buffer_id, BOSPacket** out_packet) {
    if (!g_bospectra_buffer_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_packet) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (buffer_id == 0 || buffer_id > BOSPECTRA_MAX_RING_BUFFERS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = buffer_id - 1;
    if (!g_bospectra_buffers[idx].is_allocated) return BOSPECTRA_ERR_HANDLE_INVALID;
    if (g_bospectra_buffers[idx].count == 0) {
        *out_packet = NULL;
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    *out_packet = g_bospectra_buffers[idx].packets[g_bospectra_buffers[idx].head];
    return BOSPECTRA_SUCCESS;
}

uint32_t bospectra_buffer_get_count(bospectra_buffer_id_t buffer_id) {
    if (buffer_id == 0 || buffer_id > BOSPECTRA_MAX_RING_BUFFERS) return 0;
    uint32_t idx = buffer_id - 1;
    if (!g_bospectra_buffers[idx].is_allocated) return 0;
    return g_bospectra_buffers[idx].count;
}
