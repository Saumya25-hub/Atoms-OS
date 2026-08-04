#ifndef PACKET_POOL_H
#define PACKET_POOL_H

#include "../../packet/bospectra_packet.h"

#define BOSPECTRA_PACKET_POOL_SIZE 128U
#define BOSPECTRA_DEFAULT_PACKET_PAYLOAD_SIZE 8192U

void              bospectra_packet_pool_init(void);
void              bospectra_packet_pool_shutdown(void);
bospectra_error_t bospectra_packet_pool_acquire(size_t required_size, BOSPacket** out_pkt);
bospectra_error_t bospectra_packet_pool_release(BOSPacket* pkt);
void              bospectra_packet_pool_get_counts(uint32_t* active, uint32_t* peak, uint32_t* capacity);

#endif // PACKET_POOL_H
