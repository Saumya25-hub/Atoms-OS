#ifndef KLOG_H
#define KLOG_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ⚛️ KLOG — KERNEL RING BUFFER LOGGING SUBSYSTEM V1.0
 * Decoupled in-memory circular ring buffer for driver/kernel logging (/dev/kmsg equivalent).
 * Streams logs safely to Serial UART & AMDE Network with ZERO Framebuffer access.
 */

#define KLOG_RING_SIZE   65536  /* 64 KB Circular Log Buffer */
#define KLOG_LINE_MAX    256

typedef enum {
    KLOG_LEVEL_DEBUG = 0,
    KLOG_LEVEL_INFO,
    KLOG_LEVEL_WARN,
    KLOG_LEVEL_ERROR,
    KLOG_LEVEL_CRITICAL
} klog_level_t;

void klog_init(void);
void klog_write(klog_level_t level, const char* subsys, const char* fmt, ...);
void klog_info(const char* subsys, const char* msg);
void klog_warn(const char* subsys, const char* msg);
void klog_error(const char* subsys, const char* msg);

uint32_t klog_read(char* out_buf, uint32_t max_bytes);
uint32_t klog_get_total_bytes(void);

#endif /* KLOG_H */
