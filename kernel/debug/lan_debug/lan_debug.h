#ifndef SIGNATURES_LAN_DEBUG_SUB_H
#define SIGNATURES_LAN_DEBUG_SUB_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define IP4_ADDR_NET(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define LAN_DEBUG_HOST_IP     IP4_ADDR_NET(192, 168, 0, 222)
#define LAN_DEBUG_TARGET_IP   IP4_ADDR_NET(192, 168, 0, 104) // Direct UDP unicast to Laptop Wi-Fi IP
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

/* Rolling Forensic Event Ring Buffer (Last 256 Events) */
#define FORENSIC_EVENT_RING_CAPACITY 256
#define FORENSIC_EVENT_TEXT_LEN      96

typedef struct {
    uint64_t timestamp_ticks;
    char subsys[16];
    char text[FORENSIC_EVENT_TEXT_LEN];
} ForensicTraceEvent;

void atoms_trace_event(const char* subsys, const char* fmt, ...);
void atoms_trace_dump_to_lan(void);

#endif // SIGNATURES_LAN_DEBUG_SUB_H
