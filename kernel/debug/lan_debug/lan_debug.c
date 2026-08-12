#include "kernel/debug/lan_debug/lan_debug.h"
#include "kernel/drivers/net/e1000/e1000.h"
#include "kernel/net/ethernet/ethernet.h"
#include "kernel/net/ipv4/ipv4.h"
#include "kernel/net/udp/udp.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static bool g_debuglan_active = false;
static bool g_in_debuglan_log = false;

static uint64_t g_last_rate_limit_ticks = 0;
static uint32_t g_packets_sent_this_sec = 0;
static uint16_t g_ip_id_counter = 1;

static DebugLanLogEntry g_queue[LAN_DEBUG_QUEUE_CAPACITY];
static uint32_t g_queue_head = 0;
static uint32_t g_queue_tail = 0;
static uint32_t g_queue_count = 0;

volatile uint64_t g_debuglan_tx_attempts = 0;
volatile uint64_t g_debuglan_tx_success  = 0;

static const uint8_t g_bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static inline uint32_t get_cpu_id(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1));
    return (ebx >> 24) & 0xFF;
}

static int lan_vsnprintf(char* buf, size_t size, const char* fmt, va_list args) {
    if (!buf || size == 0) return 0;
    size_t pos = 0;

    for (size_t i = 0; fmt[i] != '\0' && pos < size - 1; i++) {
        if (fmt[i] != '%') {
            buf[pos++] = fmt[i];
            continue;
        }

        i++;
        if (fmt[i] == '\0') break;

        bool is_long_long = false;
        if (fmt[i] == 'l') {
            i++;
            if (fmt[i] == 'l') {
                is_long_long = true;
                i++;
            }
        }

        if (fmt[i] == 's') {
            const char* s = va_arg(args, const char*);
            if (!s) s = "(null)";
            while (*s && pos < size - 1) {
                buf[pos++] = *s++;
            }
        } else if (fmt[i] == 'c') {
            char c = (char)va_arg(args, int);
            buf[pos++] = c;
        } else if (fmt[i] == 'd' || fmt[i] == 'i') {
            int64_t val = is_long_long ? va_arg(args, int64_t) : va_arg(args, int);
            if (val < 0) {
                buf[pos++] = '-';
                val = -val;
            }
            char num_buf[32];
            int num_pos = 0;
            if (val == 0) num_buf[num_pos++] = '0';
            while (val > 0) {
                num_buf[num_pos++] = '0' + (val % 10);
                val /= 10;
            }
            while (num_pos > 0 && pos < size - 1) {
                buf[pos++] = num_buf[--num_pos];
            }
        } else if (fmt[i] == 'u') {
            uint64_t val = is_long_long ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
            char num_buf[32];
            int num_pos = 0;
            if (val == 0) num_buf[num_pos++] = '0';
            while (val > 0) {
                num_buf[num_pos++] = '0' + (val % 10);
                val /= 10;
            }
            while (num_pos > 0 && pos < size - 1) {
                buf[pos++] = num_buf[--num_pos];
            }
        } else if (fmt[i] == 'x' || fmt[i] == 'X' || fmt[i] == 'p') {
            uint64_t val = (fmt[i] == 'p') ? (uint64_t)va_arg(args, void*) : (is_long_long ? va_arg(args, uint64_t) : va_arg(args, uint32_t));
            if (fmt[i] == 'p') {
                if (pos < size - 2) {
                    buf[pos++] = '0';
                    buf[pos++] = 'x';
                }
            }
            const char hex_digits[] = "0123456789ABCDEF";
            char num_buf[32];
            int num_pos = 0;
            if (val == 0) num_buf[num_pos++] = '0';
            while (val > 0) {
                num_buf[num_pos++] = hex_digits[val & 0xF];
                val >>= 4;
            }
            while (num_pos > 0 && pos < size - 1) {
                buf[pos++] = num_buf[--num_pos];
            }
        } else if (fmt[i] == '%') {
            buf[pos++] = '%';
        } else {
            buf[pos++] = fmt[i];
        }
    }

    buf[pos] = '\0';
    return (int)pos;
}

static int lan_snprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int res = lan_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return res;
}

static void detect_subsystem(const char* text, char out_subsys[16]) {
    if (!text || !out_subsys) return;
    
    if (strstr(text, "USB") || strstr(text, "usb") || strstr(text, "XHCI") || strstr(text, "xhci")) {
        strcpy(out_subsys, "USB");
    } else if (strstr(text, "PMM") || strstr(text, "VMM") || strstr(text, "HEAP") || strstr(text, "MEMORY") || strstr(text, "MEM")) {
        strcpy(out_subsys, "MEMORY");
    } else if (strstr(text, "VFS") || strstr(text, "vfs") || strstr(text, "FAT32") || strstr(text, "NTFS")) {
        strcpy(out_subsys, "VFS");
    } else if (strstr(text, "PANIC") || strstr(text, "panic") || strstr(text, "ASSERT")) {
        strcpy(out_subsys, "PANIC");
    } else if (strstr(text, "SCHED") || strstr(text, "sched") || strstr(text, "Thread")) {
        strcpy(out_subsys, "SCHED");
    } else if (strstr(text, "HEARTBEAT") || strstr(text, "heartbeat")) {
        strcpy(out_subsys, "HEARTBEAT");
    } else if (strstr(text, "E1000") || strstr(text, "ETH") || strstr(text, "IPV4") || strstr(text, "UDP")) {
        strcpy(out_subsys, "NET");
    } else if (strstr(text, "DISPLAY") || strstr(text, "BWE") || strstr(text, "GUI")) {
        strcpy(out_subsys, "DISPLAY");
    } else {
        strcpy(out_subsys, "KERNEL");
    }
}

#include <kernel/net/net_framework.h>

bool debuglan_send_raw(const void* data, uint32_t length) {
    if (!data || length == 0) return false;
    net_device_t* dev = net_device_get_default();
    if (dev && dev->ops.xmit) {
        return dev->ops.xmit(dev, data, (uint16_t)length);
    }
    return false;
}

void debuglan_init(void) {
    extern void com1_puts(const char* s);

    display_print("\n=== LAN DEBUG TELEMETRY ENGINE INIT ===\n");
    display_print("[LANDBG] Using NETLIB Multi-Family Realtek Master Driver\n");

    net_device_t* dev = net_device_get_default();
    display_print("[LANDBG] Net Device: ");
    if (dev) {
        display_print(dev->name ? dev->name : "NETDEV");
        display_print(" [READY]");
    } else {
        display_print("NULL (NO_DEVICE)");
    }
    display_print("\n");

    g_queue_head = 0;
    g_queue_tail = 0;
    g_queue_count = 0;
    g_last_rate_limit_ticks = timer_get_ticks();
    g_packets_sent_this_sec = 0;
    g_ip_id_counter = 1;
    g_debuglan_active = (dev != NULL && dev->ops.xmit != NULL);

    display_print("[LANDBG] ACTIVE: ");
    display_print(g_debuglan_active ? "YES" : "NO *** PACKETS WILL NOT SEND! ***");
    display_print("\n");
    display_print("[LANDBG] Target: 192.168.2.1:9999\n");
    display_print("=== LAN DEBUG ENGINE INIT DONE ===\n\n");

    com1_puts(g_debuglan_active ? "[LANDBG] ACTIVE=YES\r\n" : "[LANDBG] ACTIVE=NO\r\n");
}

bool debuglan_active(void) {
    return g_debuglan_active;
}

void debuglan_log_subsys(const char* subsys, const char* fmt, ...) {
    if (!g_debuglan_active || g_in_debuglan_log || !fmt) return;
    g_in_debuglan_log = true;

    if (g_queue_count >= LAN_DEBUG_QUEUE_CAPACITY) {
        g_queue_head = (g_queue_head + 1) % LAN_DEBUG_QUEUE_CAPACITY;
        g_queue_count--;
    }

    DebugLanLogEntry* entry = &g_queue[g_queue_tail];
    entry->ticks = timer_get_ticks();
    entry->cpu_id = get_cpu_id();

    if (subsys && subsys[0] != '\0') {
        strncpy(entry->subsys, subsys, 15);
        entry->subsys[15] = '\0';
    } else {
        detect_subsystem(fmt, entry->subsys);
    }

    va_list args;
    va_start(args, fmt);
    entry->length = (uint16_t)lan_vsnprintf(entry->text, sizeof(entry->text), fmt, args);
    va_end(args);

    g_queue_tail = (g_queue_tail + 1) % LAN_DEBUG_QUEUE_CAPACITY;
    g_queue_count++;

    g_in_debuglan_log = false;
    debuglan_flush();
}

void debuglan_log(const char* fmt, ...) {
    if (!g_debuglan_active || g_in_debuglan_log || !fmt) return;
    g_in_debuglan_log = true;

    if (g_queue_count >= LAN_DEBUG_QUEUE_CAPACITY) {
        g_queue_head = (g_queue_head + 1) % LAN_DEBUG_QUEUE_CAPACITY;
        g_queue_count--;
    }

    DebugLanLogEntry* entry = &g_queue[g_queue_tail];
    entry->ticks = timer_get_ticks();
    entry->cpu_id = get_cpu_id();

    va_list args;
    va_start(args, fmt);
    entry->length = (uint16_t)lan_vsnprintf(entry->text, sizeof(entry->text), fmt, args);
    va_end(args);

    detect_subsystem(entry->text, entry->subsys);

    g_queue_tail = (g_queue_tail + 1) % LAN_DEBUG_QUEUE_CAPACITY;
    g_queue_count++;

    g_in_debuglan_log = false;
    debuglan_flush();
}

void debuglan_flush(void) {
    if (!g_debuglan_active || g_queue_count == 0) return;

    uint64_t now = timer_get_ticks();
    if (now - g_last_rate_limit_ticks >= 1000) {
        g_last_rate_limit_ticks = now;
        g_packets_sent_this_sec = 0;
    }

    net_device_t* dev = net_device_get_default();
    if (!dev || !dev->ops.xmit) return;

    uint8_t src_mac[6] = {0};
    memcpy(src_mac, dev->mac_addr, 6);
    if (src_mac[0] == 0 && src_mac[1] == 0 && src_mac[2] == 0) {
        src_mac[0] = 0xA0; src_mac[1] = 0xAD; src_mac[2] = 0x9F;
        src_mac[3] = 0xC5; src_mac[4] = 0x81; src_mac[5] = 0x27;
    }

    while (g_queue_count > 0 && g_packets_sent_this_sec < LAN_DEBUG_MAX_PACKETS_PER_SEC) {
        DebugLanLogEntry* entry = &g_queue[g_queue_head];

        // Format Packet Payload
        char payload_buf[768];
        int payload_len = lan_snprintf(payload_buf, sizeof(payload_buf),
                                       "[%s]\n%s",
                                       entry->subsys, entry->text);

        if (payload_len <= 0) {
            g_queue_head = (g_queue_head + 1) % LAN_DEBUG_QUEUE_CAPACITY;
            g_queue_count--;
            continue;
        }

        // Build Raw Ethernet (14B) + IPv4 (20B) + UDP (8B) + Payload
        uint8_t frame_buf[ETH_MAX_LEN];
        uint16_t total_udp_len = 8 + payload_len;
        uint16_t total_ip_len = 20 + total_udp_len;
        uint16_t total_frame_len = 14 + total_ip_len;

        memset(frame_buf, 0, sizeof(frame_buf));

        struct eth_hdr* eth = (struct eth_hdr*)frame_buf;
        memcpy(eth->dest_mac, g_bcast_mac, 6);
        memcpy(eth->src_mac, src_mac, 6);
        eth->ethertype = htons(ETH_TYPE_IPV4);

        struct ip_hdr* ip = (struct ip_hdr*)(frame_buf + 14);
        ip->ihl_ver = (4 << 4) | 5;
        ip->tos = 0;
        ip->total_len = htons(total_ip_len);
        ip->id = htons(g_ip_id_counter++);
        ip->frag_off = htons(0x4000);
        ip->ttl = IP_DEFAULT_TTL;
        ip->proto = IP_PROTO_UDP;
        ip->checksum = 0;
        ip->src_ip = LAN_DEBUG_HOST_IP;
        ip->dest_ip = LAN_DEBUG_TARGET_IP;

        uint32_t sum = 0;
        uint16_t* ip_words = (uint16_t*)ip;
        for (int w = 0; w < 10; w++) {
            sum += ntohs(ip_words[w]);
        }
        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        ip->checksum = htons((uint16_t)(~sum));

        struct udp_hdr* udp = (struct udp_hdr*)(frame_buf + 34);
        udp->src_port = htons(LAN_DEBUG_PORT);
        udp->dest_port = htons(LAN_DEBUG_PORT);
        udp->length = htons(total_udp_len);
        udp->checksum = 0;

        memcpy(frame_buf + 42, payload_buf, payload_len);

        uint16_t send_len = (total_frame_len < ETH_MIN_LEN) ? ETH_MIN_LEN : total_frame_len;
        g_debuglan_tx_attempts++;
        if (debuglan_send_raw(frame_buf, send_len)) {
            g_packets_sent_this_sec++;
            g_debuglan_tx_success++;
        }

        g_queue_head = (g_queue_head + 1) % LAN_DEBUG_QUEUE_CAPACITY;
        g_queue_count--;
    }
}

void kprintf(const char* fmt, ...) {
    if (!fmt) return;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    lan_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    display_print(buf);
}
