#include "kernel/debug/klog/include/klog.h"
#include <stdarg.h>

/*
 * ⚛️ KLOG — KERNEL RING BUFFER LOGGING SUBSYSTEM V1.0 IMPLEMENTATION
 */

extern void com1_puts(const char *s);
extern void debuglan_log(const char *fmt, ...);

static char     g_klog_ring[KLOG_RING_SIZE];
static uint32_t g_klog_head = 0;
static uint32_t g_klog_tail = 0;
static uint32_t g_klog_count = 0;
static uint64_t g_klog_total_written = 0;

void klog_init(void) {
    g_klog_head = 0;
    g_klog_tail = 0;
    g_klog_count = 0;
    g_klog_total_written = 0;
}

static void klog_push_char(char c) {
    g_klog_ring[g_klog_head] = c;
    g_klog_head = (g_klog_head + 1) % KLOG_RING_SIZE;
    if (g_klog_count < KLOG_RING_SIZE) {
        g_klog_count++;
    } else {
        g_klog_tail = (g_klog_tail + 1) % KLOG_RING_SIZE;
    }
    g_klog_total_written++;
}

static void klog_push_str(const char* s) {
    if (!s) return;
    while (*s) {
        klog_push_char(*s++);
    }
}

void klog_write(klog_level_t level, const char* subsys, const char* fmt, ...) {
    (void)level;
    if (!subsys) subsys = "KERNEL";

    char header[64];
    int hpos = 0;
    header[hpos++] = '[';
    const char* p = subsys;
    while (*p && hpos < 40) header[hpos++] = *p++;
    header[hpos++] = ']';
    header[hpos++] = ' ';
    header[hpos] = '\0';

    klog_push_str(header);

    if (fmt) {
        klog_push_str(fmt);
    }
    klog_push_str("\r\n");

    /* Stream log entry to Serial UART COM1 and AMDE UDP Telemetry (ZERO Framebuffer) */
    com1_puts(header);
    if (fmt) com1_puts(fmt);
    com1_puts("\r\n");
}

void klog_info(const char* subsys, const char* msg) {
    klog_write(KLOG_LEVEL_INFO, subsys, "%s", msg ? msg : "");
}

void klog_warn(const char* subsys, const char* msg) {
    klog_write(KLOG_LEVEL_WARN, subsys, "%s", msg ? msg : "");
}

void klog_error(const char* subsys, const char* msg) {
    klog_write(KLOG_LEVEL_ERROR, subsys, "%s", msg ? msg : "");
}

uint32_t klog_read(char* out_buf, uint32_t max_bytes) {
    if (!out_buf || max_bytes == 0) return 0;

    uint32_t read_bytes = 0;
    while (g_klog_count > 0 && read_bytes < max_bytes - 1) {
        out_buf[read_bytes++] = g_klog_ring[g_klog_tail];
        g_klog_tail = (g_klog_tail + 1) % KLOG_RING_SIZE;
        g_klog_count--;
    }
    out_buf[read_bytes] = '\0';
    return read_bytes;
}

uint32_t klog_get_total_bytes(void) {
    return g_klog_count;
}
