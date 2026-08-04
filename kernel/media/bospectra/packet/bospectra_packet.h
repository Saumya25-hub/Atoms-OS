#ifndef BOSPECTRA_PACKET_H
#define BOSPECTRA_PACKET_H

#include "../include/bospectra_types.h"

#define BOSPECTRA_PACKET_FLAG_KEYFRAME 0x0001U
#define BOSPECTRA_PACKET_FLAG_CORRUPT  0x0002U
#define BOSPECTRA_PACKET_FLAG_DISCONT  0x0004U

typedef struct BOSPacket {
    bospectra_stream_id_t stream_id;
    uint64_t              pts;           // Presentation Timestamp (microseconds)
    uint64_t              dts;           // Decode Timestamp (microseconds)
    uint64_t              duration_us;   // Duration in microseconds
    uint32_t              flags;         // Keyframe, discontinuity, corrupt flags
    uint8_t*              data;          // Payload byte array
    size_t                size;          // Payload size in bytes
    uint32_t              ref_count;     // Reference counter for zero-copy sharing
} BOSPacket;

void              bospectra_packet_subsystem_init(void);
void              bospectra_packet_subsystem_shutdown(void);
BOSPacket*        bospectra_packet_alloc(size_t payload_size);
BOSPacket*        bospectra_packet_clone(BOSPacket* pkt);
bospectra_error_t bospectra_packet_free(BOSPacket* pkt);
bool              bospectra_packet_is_valid(const BOSPacket* pkt);

#endif // BOSPECTRA_PACKET_H
