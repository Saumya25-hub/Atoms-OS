#include "kernel/debug/aipdebug/aipdebug.h"
#include "kernel/debug/lan_debug/lan_debug.h"
#include "kernel/net/net_framework.h"
#include "kernel/core/lib/include/string.h"
#include "arch/x86_64/io/port_io.h"

extern void com1_puts(const char* s);
extern bool debuglan_send_raw(const void* data, uint32_t length);

#pragma pack(push, 1)
struct eth_header {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype;
};

struct ip_header {
    uint8_t  ihl_ver;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
};

struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
};
#pragma pack(pop)

/* Static preallocated ring buffer: 2048 records * 32B = 64 KB */
static AIPDEventRecord s_ring[AIPD_RING_CAPACITY];
static volatile uint32_t s_head = 0;
static volatile uint32_t s_tail = 0;
static volatile uint32_t s_dropped_events = 0;

static uint16_t s_active_profile = AIPD_PROF_ALL;
static uint32_t s_session_id = 1;
static uint32_t s_seq_no = 0;
static uint16_t s_tx_counter = 100;
static uint64_t s_tx_start_tsc = 0;
static uint16_t s_ip_id_counter = 1;

static inline uint64_t aipd_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline uint16_t swap16(uint16_t v) {
    return (v >> 8) | (v << 8);
}

void aipd_detect_cpu_brand(char out_brand[49]) {
    if (!out_brand) return;
    uint32_t eax, ebx, ecx, edx;
    uint32_t* ptr = (uint32_t*)out_brand;

    for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
        __asm__ volatile("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(leaf));
        *ptr++ = eax;
        *ptr++ = ebx;
        *ptr++ = ecx;
        *ptr++ = edx;
    }
    out_brand[48] = '\0';
}

void aipd_init(uint16_t profile_mask) {
    s_head = 0;
    s_tail = 0;
    s_dropped_events = 0;
    s_active_profile = profile_mask;
    s_tx_start_tsc = aipd_rdtsc();
    com1_puts("[AIPDEBUG] Engine Initialized (Port: 9997, Zero-Heap Ring: 2048 records)\r\n");
}

uint32_t aipd_get_dropped_count(void) {
    return s_dropped_events;
}

static void aipd_ring_push(const AIPDEventRecord* rec) {
    if (!rec) return;

    uint32_t next_head = (s_head + 1) % AIPD_RING_CAPACITY;
    if (next_head == s_tail) {
        /* Ring buffer full: bump drop counter and discard oldest */
        s_dropped_events++;
        s_tail = (s_tail + 1) % AIPD_RING_CAPACITY;
    }

    s_ring[s_head] = *rec;
    s_head = next_head;
}

uint16_t aipd_tx_begin(AIPDTransactionType tx_type, uint32_t target_addr) {
    uint16_t tx_id = ++s_tx_counter;
    s_tx_start_tsc = aipd_rdtsc();

    AIPDEventRecord rec;
    rec.tsc = s_tx_start_tsc;
    rec.time_us_delta = 0;
    rec.subsystem_id = AIPD_SUBSYS_CORE;
    rec.component_id = AIPD_COMP_NONE;
    rec.truth_class = AIPD_TRUTH_OBSERVED;
    rec.event_type = AIPD_EVT_TX_BEGIN;
    rec.transaction_id = tx_id;
    rec.transaction_type = tx_type;
    rec.address_or_port = target_addr;
    rec.raw_value_before = 0;
    rec.raw_value_after = 0;
    rec.result_status = AIPD_RES_PENDING;

    aipd_ring_push(&rec);
    return tx_id;
}

void aipd_tx_record(uint16_t tx_id, AIPDSubsystem subsys, AIPDComponent comp,
                    AIPDEventType evt, AIPDTruthClass truth,
                    uint32_t addr, uint32_t before, uint32_t after,
                    uint16_t status) {
    uint64_t now_tsc = aipd_rdtsc();
    uint64_t diff_cycles = (now_tsc >= s_tx_start_tsc) ? (now_tsc - s_tx_start_tsc) : 0;
    /* Approximate delta us assuming ~3GHz or timer base */
    uint16_t delta_us = (uint16_t)(diff_cycles / 3000);

    AIPDEventRecord rec;
    rec.tsc = now_tsc;
    rec.time_us_delta = delta_us;
    rec.subsystem_id = subsys;
    rec.component_id = comp;
    rec.truth_class = truth;
    rec.event_type = evt;
    rec.transaction_id = tx_id;
    rec.transaction_type = 0;
    rec.address_or_port = addr;
    rec.raw_value_before = before;
    rec.raw_value_after = after;
    rec.result_status = status;

    aipd_ring_push(&rec);
}

void aipd_tx_end(uint16_t tx_id, AIPDTransactionResult result) {
    uint64_t now_tsc = aipd_rdtsc();
    uint64_t diff_cycles = (now_tsc >= s_tx_start_tsc) ? (now_tsc - s_tx_start_tsc) : 0;
    uint16_t delta_us = (uint16_t)(diff_cycles / 3000);

    AIPDEventRecord rec;
    rec.tsc = now_tsc;
    rec.time_us_delta = delta_us;
    rec.subsystem_id = AIPD_SUBSYS_CORE;
    rec.component_id = AIPD_COMP_NONE;
    rec.truth_class = AIPD_TRUTH_OBSERVED;
    rec.event_type = AIPD_EVT_TX_END;
    rec.transaction_id = tx_id;
    rec.transaction_type = 0;
    rec.address_or_port = 0;
    rec.raw_value_before = 0;
    rec.raw_value_after = 0;
    rec.result_status = result;

    aipd_ring_push(&rec);
}

void aipd_flush(void) {
    net_device_t* dev = net_device_get_default();
    if (!dev || !dev->ops.xmit) return;

    uint8_t src_mac[6];
    memcpy(src_mac, dev->mac_addr, 6);
    static const uint8_t bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    while (s_tail != s_head) {
        /* Count how many records to bundle into this packet */
        uint32_t count = 0;
        uint32_t temp_tail = s_tail;
        while (temp_tail != s_head && count < AIPD_MAX_RECORDS_PER_PKT) {
            count++;
            temp_tail = (temp_tail + 1) % AIPD_RING_CAPACITY;
        }

        if (count == 0) break;

        uint8_t frame[1514];
        uint16_t aipd_payload_len = sizeof(AIPDPacketHeader) + (count * sizeof(AIPDEventRecord));
        uint16_t total_udp_len = sizeof(struct udp_header) + aipd_payload_len;
        uint16_t total_ip_len = sizeof(struct ip_header) + total_udp_len;
        uint16_t total_frame_len = sizeof(struct eth_header) + total_ip_len;

        memset(frame, 0, total_frame_len);

        /* Ethernet Header */
        struct eth_header* eth = (struct eth_header*)frame;
        memcpy(eth->dest_mac, bcast_mac, 6);
        memcpy(eth->src_mac, src_mac, 6);
        eth->ethertype = swap16(0x0800); /* IPv4 */

        /* IPv4 Header */
        struct ip_header* ip = (struct ip_header*)(frame + sizeof(struct eth_header));
        ip->ihl_ver = 0x45;
        ip->tos = 0;
        ip->total_len = swap16(total_ip_len);
        ip->id = swap16(s_ip_id_counter++);
        ip->frag_off = swap16(0x4000);
        ip->ttl = 64;
        ip->proto = 17; /* UDP */
        ip->src_ip = LAN_DEBUG_HOST_IP;
        ip->dest_ip = LAN_DEBUG_TARGET_IP;

        /* Checksum */
        uint32_t csum = 0;
        uint16_t* words = (uint16_t*)ip;
        for (int i = 0; i < 10; i++) csum += swap16(words[i]);
        while (csum >> 16) csum = (csum & 0xFFFF) + (csum >> 16);
        ip->checksum = swap16((uint16_t)(~csum));

        /* UDP Header */
        struct udp_header* udp = (struct udp_header*)(frame + sizeof(struct eth_header) + sizeof(struct ip_header));
        udp->src_port = swap16(AIPD_UDP_PORT);
        udp->dest_port = swap16(AIPD_UDP_PORT);
        udp->length = swap16(total_udp_len);
        udp->checksum = 0;

        /* AIPD Packet Header */
        uint8_t* payload = frame + sizeof(struct eth_header) + sizeof(struct ip_header) + sizeof(struct udp_header);
        AIPDPacketHeader* hdr = (AIPDPacketHeader*)payload;
        hdr->magic = AIPD_MAGIC;
        hdr->session_id = s_session_id;
        hdr->sequence_number = ++s_seq_no;
        hdr->record_count = (uint16_t)count;
        hdr->active_profile = s_active_profile;

        /* Copy Event Records */
        AIPDEventRecord* rec_dst = (AIPDEventRecord*)(payload + sizeof(AIPDPacketHeader));
        for (uint32_t i = 0; i < count; i++) {
            rec_dst[i] = s_ring[s_tail];
            s_tail = (s_tail + 1) % AIPD_RING_CAPACITY;
        }

        uint16_t send_len = (total_frame_len < 60) ? 60 : total_frame_len;
        debuglan_send_raw(frame, send_len);

        /* Brief delay between packet bursts to prevent physical switch buffer drops */
        for (volatile int d = 0; d < 2000; d++) {
            io_in8(0x80);
        }
    }
}
