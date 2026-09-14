#include "packet_pool.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    BOSPacket packet;
    uint32_t  pool_idx;
    bool      in_use;
} PacketPoolSlot;

static PacketPoolSlot g_packet_pool[BOSPECTRA_PACKET_POOL_SIZE];
static uint32_t g_packet_pool_active = 0;
static uint32_t g_packet_pool_peak = 0;
static bool     g_packet_pool_initialized = false;

void bospectra_packet_pool_init(void) {
    memset(g_packet_pool, 0, sizeof(g_packet_pool));

    for (uint32_t i = 0; i < BOSPECTRA_PACKET_POOL_SIZE; i++) {
        g_packet_pool[i].pool_idx = i;
        g_packet_pool[i].in_use = false;
        g_packet_pool[i].packet.data = NULL;
        g_packet_pool[i].packet.size = 0;
        g_packet_pool[i].packet.ref_count = 0;
    }

    g_packet_pool_active = 0;
    g_packet_pool_peak = 0;
    g_packet_pool_initialized = true;
}

void bospectra_packet_pool_shutdown(void) {
    for (uint32_t i = 0; i < BOSPECTRA_PACKET_POOL_SIZE; i++) {
        if (g_packet_pool[i].packet.data) {
            bospectra_mem_free(g_packet_pool[i].packet.data);
            g_packet_pool[i].packet.data = NULL;
        }
    }
    memset(g_packet_pool, 0, sizeof(g_packet_pool));
    g_packet_pool_active = 0;
    g_packet_pool_peak = 0;
    g_packet_pool_initialized = false;
}

bospectra_error_t bospectra_packet_pool_acquire(size_t required_size, BOSPacket** out_pkt) {
    if (!g_packet_pool_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    size_t alloc_sz = (required_size > 0) ? required_size : BOSPECTRA_DEFAULT_PACKET_PAYLOAD_SIZE;

    for (uint32_t i = 0; i < BOSPECTRA_PACKET_POOL_SIZE; i++) {
        if (!g_packet_pool[i].in_use) {
            PacketPoolSlot* slot = &g_packet_pool[i];

            BOSPacket* pkt = &slot->packet;
            if (!pkt->data || pkt->size < alloc_sz) {
                if (pkt->data) {
                    bospectra_mem_free(pkt->data);
                    pkt->data = NULL;
                }
                pkt->data = (uint8_t*)bospectra_mem_alloc_aligned(alloc_sz, BOSPECTRA_DEFAULT_ALIGNMENT, "PktBuf");
                if (!pkt->data) {
                    return BOSPECTRA_ERR_OUT_OF_MEMORY;
                }
                pkt->size = alloc_sz;
            }

            slot->in_use = true;
            pkt->stream_id = 0;
            pkt->pts = 0;
            pkt->dts = 0;
            pkt->duration_us = 0;
            pkt->flags = 0;
            pkt->ref_count = 1;

            g_packet_pool_active++;
            if (g_packet_pool_active > g_packet_pool_peak) {
                g_packet_pool_peak = g_packet_pool_active;
            }

            *out_pkt = pkt;
            return BOSPECTRA_SUCCESS;
        }
    }

    *out_pkt = NULL;
    return BOSPECTRA_ERR_OUT_OF_MEMORY;
}

bospectra_error_t bospectra_packet_pool_release(BOSPacket* pkt) {
    if (!g_packet_pool_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_PACKET_POOL_SIZE; i++) {
        if (&g_packet_pool[i].packet == pkt) {
            if (!g_packet_pool[i].in_use) {
                return BOSPECTRA_ERR_HANDLE_INVALID; // Double free protection
            }
            g_packet_pool[i].in_use = false;
            g_packet_pool[i].packet.ref_count = 0;
            if (g_packet_pool_active > 0) g_packet_pool_active--;
            return BOSPECTRA_SUCCESS;
        }
    }

    return BOSPECTRA_ERR_HANDLE_INVALID;
}

void bospectra_packet_pool_get_counts(uint32_t* active, uint32_t* peak, uint32_t* capacity) {
    if (active) *active = g_packet_pool_active;
    if (peak) *peak = g_packet_pool_peak;
    if (capacity) *capacity = BOSPECTRA_PACKET_POOL_SIZE;
}
