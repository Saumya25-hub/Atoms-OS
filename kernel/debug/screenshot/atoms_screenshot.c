#include "atoms_screenshot.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/debug/lan_debug/lan_debug.h"
#include "kernel/net/net_framework.h"
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

#define SCREENSHOT_BURST_CHUNKS 4

typedef struct {
    bool     active;
    uint32_t session_id;
    uint16_t total_chunks;
    uint16_t current_chunk;
    uint32_t current_file_offset;
    uint32_t total_bmp_size;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint64_t fb_addr;
    uint32_t row_bytes;
    uint8_t  bmp_hdr_buf[54];
} ScreenshotStreamState;

static ScreenshotStreamState s_stream = {0};

static inline uint16_t swap16(uint16_t v) {
    return (v >> 8) | (v << 8);
}

static void screenshot_com1_put_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }
    com1_puts(&buf[pos + 1]);
}

void atoms_screenshot_init(void) {
    com1_puts("[SCREENSHOT] Cooperative Non-Blocking Engine Initialized (Port: 9998)\r\n");
}

bool atoms_screenshot_is_busy(void) {
    return s_stream.active;
}

bool atoms_screenshot_request(uint32_t session_id) {
    if (s_stream.active) {
        return false; // Already streaming an active screenshot
    }

    extern net_device_t* net_device_get_default(void);
    net_device_t* dev = net_device_get_default();
    if (!dev || !dev->ops.xmit) {
        return false;
    }

    uint64_t fb_addr = g_abde.framebuffer;
    uint32_t width   = g_abde.width ? g_abde.width : 1024;
    uint32_t height  = g_abde.height ? g_abde.height : 768;
    uint32_t pitch   = g_abde.pitch ? g_abde.pitch : (width * 4);

    if (!fb_addr) {
        com1_puts("[SCREENSHOT] ERROR: No valid framebuffer address in g_abde!\r\n");
        return false;
    }

    uint32_t row_bytes   = width * 4;
    uint32_t pixel_bytes = row_bytes * height;
    uint32_t total_bmp_size = 54 + pixel_bytes;

    s_stream.session_id = session_id;
    s_stream.width = width;
    s_stream.height = height;
    s_stream.pitch = pitch;
    s_stream.fb_addr = fb_addr;
    s_stream.row_bytes = row_bytes;
    s_stream.total_bmp_size = total_bmp_size;
    s_stream.total_chunks = (uint16_t)((total_bmp_size + SCREENSHOT_CHUNK_PAYLOAD - 1) / SCREENSHOT_CHUNK_PAYLOAD);
    s_stream.current_chunk = 0;
    s_stream.current_file_offset = 0;

    // Construct 54-byte BMP Header
    BmpFileHeader *bf = (BmpFileHeader*)s_stream.bmp_hdr_buf;
    bf->bfType      = 0x4D42; // "BM"
    bf->bfSize      = total_bmp_size;
    bf->bfReserved1 = 0;
    bf->bfReserved2 = 0;
    bf->bfOffBits   = 54;

    BmpInfoHeader *bi = (BmpInfoHeader*)(s_stream.bmp_hdr_buf + 14);
    bi->biSize          = 40;
    bi->biWidth         = (int32_t)width;
    bi->biHeight        = (int32_t)height; // Positive = bottom-up
    bi->biPlanes        = 1;
    bi->biBitCount      = 32;
    bi->biCompression   = 0; // BI_RGB
    bi->biSizeImage     = pixel_bytes;
    bi->biXPelsPerMeter = 2835;
    bi->biYPelsPerMeter = 2835;
    bi->biClrUsed       = 0;
    bi->biClrImportant  = 0;

    s_stream.active = true;

    com1_puts("[SCREENSHOT] ARMED COOPERATIVE STREAM: Session=");
    screenshot_com1_put_dec(session_id);
    com1_puts(" TotalChunks=");
    screenshot_com1_put_dec(s_stream.total_chunks);
    com1_puts("\r\n");

    return true;
}

bool atoms_screenshot_step(void) {
    if (!s_stream.active) return false;

    extern net_device_t* net_device_get_default(void);
    net_device_t* dev = net_device_get_default();
    if (!dev || !dev->ops.xmit) {
        s_stream.active = false;
        return false;
    }

    uint8_t packet_buf[1536];
    uint8_t bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t src_mac[6] = {0x00, 0xE0, 0x4C, 0x68, 0x00, 0x01};
    if (dev) {
        for (int i = 0; i < 6; i++) src_mac[i] = dev->mac_addr[i];
    }
    static uint16_t s_ip_id = 0x5000;

    for (uint32_t b = 0; b < SCREENSHOT_BURST_CHUNKS && s_stream.current_chunk < s_stream.total_chunks; b++) {
        uint16_t chunk_idx = s_stream.current_chunk;
        uint32_t current_file_offset = s_stream.current_file_offset;
        uint32_t bytes_remaining = s_stream.total_bmp_size - current_file_offset;
        uint16_t chunk_data_len = (bytes_remaining < SCREENSHOT_CHUNK_PAYLOAD) ? (uint16_t)bytes_remaining : SCREENSHOT_CHUNK_PAYLOAD;

        // Populate Ethernet Header (14 bytes)
        struct eth_header *eth = (struct eth_header*)packet_buf;
        for (int i = 0; i < 6; i++) {
            eth->dest_mac[i] = bcast_mac[i];
            eth->src_mac[i]  = src_mac[i];
        }
        eth->ethertype = swap16(0x0800); // IPv4

        // Populate IP Header (20 bytes)
        struct ip_header *ip = (struct ip_header*)(packet_buf + 14);
        uint16_t total_udp_len = 8 + sizeof(ScreenshotChunkHeader) + chunk_data_len;
        uint16_t total_ip_len  = 20 + total_udp_len;

        ip->ihl_ver   = (4 << 4) | 5;
        ip->tos       = 0;
        ip->total_len = swap16(total_ip_len);
        ip->id        = swap16(s_ip_id++);
        ip->frag_off  = swap16(0x4000); // Don't Fragment
        ip->ttl       = 64;
        ip->proto     = 17; // UDP
        ip->checksum  = 0;
        ip->src_ip    = LAN_DEBUG_HOST_IP;   // 192.168.2.100
        ip->dest_ip   = LAN_DEBUG_TARGET_IP; // 192.168.2.1

        // Calculate IP checksum
        uint32_t csum = 0;
        uint16_t *ip_words = (uint16_t*)ip;
        for (int w = 0; w < 10; w++) {
            csum += swap16(ip_words[w]);
        }
        while (csum >> 16) {
            csum = (csum & 0xFFFF) + (csum >> 16);
        }
        ip->checksum = swap16((uint16_t)(~csum));

        // Populate UDP Header (8 bytes)
        struct udp_header *udp = (struct udp_header*)(packet_buf + 34);
        udp->src_port = swap16(SCREENSHOT_UDP_PORT);
        udp->dest_port = swap16(SCREENSHOT_UDP_PORT);
        udp->length   = swap16(total_udp_len);
        udp->checksum = 0;

        // Populate Screenshot Chunk Header (16 bytes)
        ScreenshotChunkHeader *sh = (ScreenshotChunkHeader*)(packet_buf + 42);
        sh->magic        = SCREENSHOT_MAGIC;
        sh->session_id   = s_stream.session_id;
        sh->total_chunks = s_stream.total_chunks;
        sh->chunk_index  = chunk_idx;
        sh->offset       = current_file_offset;
        sh->data_len     = chunk_data_len;
        sh->flags        = (chunk_idx == 0 ? 1 : 0) | (chunk_idx == s_stream.total_chunks - 1 ? 2 : 0);

        uint8_t *payload_dst = packet_buf + 42 + sizeof(ScreenshotChunkHeader);

        for (uint16_t byte_in_chunk = 0; byte_in_chunk < chunk_data_len; byte_in_chunk++) {
            uint32_t file_pos = current_file_offset + byte_in_chunk;
            if (file_pos < 54) {
                payload_dst[byte_in_chunk] = s_stream.bmp_hdr_buf[file_pos];
            } else {
                uint32_t pixel_offset = file_pos - 54;
                uint32_t row_from_bottom = pixel_offset / s_stream.row_bytes;
                uint32_t col_byte_offset = pixel_offset % s_stream.row_bytes;

                if (row_from_bottom < s_stream.height) {
                    uint32_t screen_y = (s_stream.height - 1) - row_from_bottom;
                    uint8_t *fb_row = (uint8_t*)(uintptr_t)(s_stream.fb_addr + screen_y * s_stream.pitch);
                    payload_dst[byte_in_chunk] = fb_row[col_byte_offset];
                } else {
                    payload_dst[byte_in_chunk] = 0;
                }
            }
        }

        uint16_t frame_len = 14 + total_ip_len;
        debuglan_send_raw(packet_buf, frame_len);
        s_stream.current_file_offset += chunk_data_len;
        s_stream.current_chunk++;
    }

    if (s_stream.current_chunk >= s_stream.total_chunks) {
        s_stream.active = false;
        com1_puts("[SCREENSHOT] TRANSMISSION COMPLETE (");
        screenshot_com1_put_dec(s_stream.total_chunks);
        com1_puts(" Chunks Transmitted)\r\n");
        return false;
    }

    return true;
}

bool atoms_screenshot_capture_and_send(uint32_t session_id) {
    return atoms_screenshot_request(session_id);
}

bool atoms_screenshot_capture_sync(uint32_t session_id) {
    s_stream.active = false; // Reset any previous partial stream
    if (!atoms_screenshot_request(session_id)) {
        return false;
    }
    com1_puts("[SCREENSHOT] Synchronous Drainage Started...\r\n");
    while (s_stream.active) {
        atoms_screenshot_step();
        for (volatile int d = 0; d < 12000; d++) {
            __asm__ volatile("pause");
        }
    }
    com1_puts("[SCREENSHOT] Synchronous Drainage Complete.\r\n");
    return true;
}
