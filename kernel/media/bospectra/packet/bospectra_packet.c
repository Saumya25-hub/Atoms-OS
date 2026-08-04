#include "bospectra_packet.h"
#include "../memory/bospectra_memory.h"
#include "../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

static bool g_bospectra_packet_initialized = false;

void bospectra_packet_subsystem_init(void) {
    g_bospectra_packet_initialized = true;
}

void bospectra_packet_subsystem_shutdown(void) {
    g_bospectra_packet_initialized = false;
}

BOSPacket* bospectra_packet_alloc(size_t payload_size) {
    if (!g_bospectra_packet_initialized) return NULL;

    BOSPacket* pkt = (BOSPacket*)bospectra_mem_alloc(sizeof(BOSPacket), "BOSPacket");
    if (!pkt) return NULL;

    memset(pkt, 0, sizeof(BOSPacket));
    pkt->ref_count = 1;

    if (payload_size > 0) {
        pkt->data = (uint8_t*)bospectra_mem_alloc_aligned(payload_size, BOSPECTRA_DEFAULT_ALIGNMENT, "BOSPacketData");
        if (!pkt->data) {
            bospectra_mem_free(pkt);
            return NULL;
        }
        pkt->size = payload_size;
    }

    return pkt;
}

BOSPacket* bospectra_packet_clone(BOSPacket* pkt) {
    if (!pkt || pkt->ref_count == 0) return NULL;
    pkt->ref_count++;
    return pkt;
}

bospectra_error_t bospectra_packet_free(BOSPacket* pkt) {
    if (!pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (pkt->ref_count == 0) return BOSPECTRA_ERR_HANDLE_INVALID;

    pkt->ref_count--;
    if (pkt->ref_count == 0) {
        if (pkt->data) {
            bospectra_mem_free(pkt->data);
            pkt->data = NULL;
        }
        bospectra_mem_free(pkt);
    }
    return BOSPECTRA_SUCCESS;
}

bool bospectra_packet_is_valid(const BOSPacket* pkt) {
    return (pkt != NULL && pkt->ref_count > 0);
}
