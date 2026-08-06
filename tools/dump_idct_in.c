#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kernel/media/bospectra/include/bospectra_types.h"
#include "kernel/media/bospectra/include/bospectra_errors.h"
#include "kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.h"
#include "kernel/media/bospectra/frame_memory/frame_pool/frame_pool.h"

// Stubs for kernel functions
void bospectra_log(const char* tag, const char* msg) { (void)tag; (void)msg; }
void bospectra_trace_str(const char* tag, const char* val) { (void)tag; (void)val; }
void bospectra_trace_u32(const char* tag, uint32_t val) { (void)tag; (void)val; }
void bospectra_trace_hex(const char* tag, uint64_t val) { (void)tag; (void)val; }
void* bospectra_mem_alloc(size_t size, const char* tag) { (void)tag; return malloc(size); }
void bospectra_mem_free(void* ptr) { free(ptr); }
void* bospectra_mem_alloc_aligned(size_t size, size_t align, const char* tag) { (void)align; (void)tag; return malloc(size); }

int main(void) {
    bospectra_frame_pool_init();

    FILE* f = fopen("build/frame30.jpg", "rb");
    if (!f) { printf("Failed to open frame30.jpg\n"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buf = malloc(sz);
    fread(buf, 1, sz, f);
    fclose(f);

    BOSPECTRA_StreamDescriptor desc;
    memset(&desc, 0, sizeof(desc));
    desc.width = 640;
    desc.height = 360;
    desc.codec_id = BOSPECTRA_CODEC_MJPEG;

    void* ctx = NULL;
    g_mjpeg_decoder_driver.open(&ctx, &desc);

    BOSPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.data = buf;
    pkt.size = sz;

    BOSFrame* pframe = NULL;
    bospectra_error_t err = g_mjpeg_decoder_driver.decode_packet(ctx, &pkt, &pframe);
    (void)err;
    (void)pframe;

    g_mjpeg_decoder_driver.close(ctx);
    free(buf);
    return 0;
}
