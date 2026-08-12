#ifndef SIGNATURES_LAN_DEBUG_SUB_H
#define SIGNATURES_LAN_DEBUG_SUB_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define IP4_ADDR_NET(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define LAN_DEBUG_HOST_IP     IP4_ADDR_NET(192, 168, 2, 100)
#define LAN_DEBUG_TARGET_IP   IP4_ADDR_NET(192, 168, 2, 1) // Direct UDP unicast to Laptop PXE Server
#define LAN_DEBUG_PORT        9999

#define LAN_DEBUG_MAX_PACKETS_PER_SEC 100
#define LAN_DEBUG_QUEUE_CAPACITY      64
#define LAN_DEBUG_MSG_SIZE            512

typedef struct {
    char text[LAN_DEBUG_MSG_SIZE];
    uint16_t length;
    uint64_t ticks;
    uint32_t cpu_id;
    char subsys[16];
} DebugLanLogEntry;

void debuglan_init(void);
void debuglan_log(const char* fmt, ...);
void debuglan_log_subsys(const char* subsys, const char* fmt, ...);
void debuglan_flush(void);
bool debuglan_active(void);

bool debuglan_send_raw(const void* data, uint32_t length);

void kprintf(const char* fmt, ...);

#endif // SIGNATURES_LAN_DEBUG_SUB_H
