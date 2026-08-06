#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kernel/media/bospectra/include/bospectra_types.h"
#include "kernel/media/bospectra/include/bospectra_errors.h"
#include "kernel/media/bospectra/decoder/mjpeg/mjpeg_decoder.h"
#include "kernel/media/bospectra/frame_memory/frame_pool/frame_pool.h"

void bospectra_log(const char* tag, const char* msg) { (void)tag; (void)msg; }
void bospectra_trace_str(const char* tag, const char* val) { (void)tag; (void)val; }
void bospectra_trace_u32(const char* tag, uint32_t val) { (void)tag; (void)val; }
void bospectra_trace_hex(const char* tag, uint64_t val) { (void)tag; (void)val; }
void* bospectra_mem_alloc(size_t size, const char* tag) { (void)tag; return malloc(size); }
void bospectra_mem_free(void* ptr) { free(ptr); }
void* bospectra_mem_alloc_aligned(size_t size, size_t align, const char* tag) { (void)align; (void)tag; return malloc(size); }

int main(void) {
    bospectra_frame_pool_init();
    FILE* f = fopen("build/frame5.jpg", "rb");
    if (!f) return 1;
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
    BOSPacket pkt; memset(&pkt, 0, sizeof(pkt)); pkt.data = buf; pkt.size = sz;
    BOSFrame* pframe = NULL;
    g_mjpeg_decoder_driver.decode_packet(ctx, &pkt, &pframe);

    if (pframe) {
        printf("Frame Decoded Successfully: W=%d H=%d Format=%d\n", pframe->width, pframe->height, pframe->format);
        printf("Luma Pitch=%d, Cb Pitch=%d, Cr Pitch=%d\n", pframe->linesize[0], pframe->linesize[1], pframe->linesize[2]);
    }

    g_mjpeg_decoder_driver.close(ctx);
    free(buf);
    return 0;
}
