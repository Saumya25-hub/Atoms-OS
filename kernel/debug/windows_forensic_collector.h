#ifndef WINDOWS_FORENSIC_COLLECTOR_H
#define WINDOWS_FORENSIC_COLLECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#define WFFP_MAGIC          0x57464650U /* "WFFP" */
#define WFFP_VERSION        1
#define WFFP_PORT           9997
#ifndef IP4_ADDR_NET
#define IP4_ADDR_NET(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#endif
#define WFFP_HOST_IP        IP4_ADDR_NET(192, 168, 2, 1)
#define WFFP_CLIENT_IP      IP4_ADDR_NET(192, 168, 2, 100)

#define WFFP_TYPE_FILE_START   1
#define WFFP_TYPE_FILE_DATA    2
#define WFFP_TYPE_FILE_END     3
#define WFFP_TYPE_FILE_ABSENT  4
#define WFFP_TYPE_SESSION_END  5

#define WFFP_MAX_PAYLOAD       1024

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;          /* WFFP_MAGIC (0x57464650) */
    uint16_t version;        /* WFFP_VERSION (1) */
    uint16_t packet_type;    /* WFFP_TYPE_* */
    uint32_t session_id;     /* Session identifier */
    uint32_t file_id;        /* Target file ID (1..N) */
    uint32_t sequence;       /* Chunk sequence index (0, 1, 2...) */
    uint64_t offset;         /* Byte offset within file */
    uint32_t payload_len;    /* Number of valid payload bytes (0..1024) */
    uint64_t total_size;     /* Total file size in bytes */
    uint32_t crc32;          /* IEEE 802.3 CRC32 of payload bytes */
    char     filename[64];   /* Basename or relative path */
} ForensicPacketHeader;

typedef struct {
    ForensicPacketHeader header;
    uint8_t payload[WFFP_MAX_PAYLOAD];
} ForensicPacket;
#pragma pack(pop)

void windows_forensic_collector_run(boot_info_t *boot_info);

#endif /* WINDOWS_FORENSIC_COLLECTOR_H */
