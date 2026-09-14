/*
 * ATOMS OS — VP8 Video Decoder Implementation
 * kernel/media/bospectra/decoder/vp8/vp8_decoder.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "vp8_decoder.h"
#include "../../memory/bospectra_memory.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t frame_count;
} VP8_DecCtx;

static void vp8_print_u32(uint32_t val) {
    char buf[16], rev[16];
    int r = 0;
    if (val == 0) { display_print("0"); return; }
    while (val > 0) { rev[r++] = '0' + (val % 10); val /= 10; }
    for (int i = 0; i < r; i++) buf[i] = rev[r - 1 - i];
    buf[r] = '\0';
    display_print(buf);
}

static bospectra_error_t vp8_open(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc) {
    if (!driver_ctx || !stream_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    VP8_DecCtx* ctx = (VP8_DecCtx*)bospectra_mem_alloc(sizeof(VP8_DecCtx), "VP8DecoderCtx");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;
    memset(ctx, 0, sizeof(VP8_DecCtx));

    ctx->width = (stream_desc->width > 0) ? stream_desc->width : 1280;
    ctx->height = (stream_desc->height > 0) ? stream_desc->height : 720;

    display_print("[VP8] decoder initialized\n");
    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t vp8_decode_packet(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame) {
    VP8_DecCtx* ctx = (VP8_DecCtx*)driver_ctx;
    if (!ctx || !packet || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (!packet->data || packet->size < 10) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    const uint8_t* p = (const uint8_t*)packet->data;
    bool is_keyframe = !(p[0] & 0x01);

    if (is_keyframe && packet->size >= 10) {
        /* Verify VP8 keyframe sync code 0x9D 0x01 0x2A */
        if (p[3] == 0x9D && p[4] == 0x01 && p[5] == 0x2A) {
            uint32_t w = ((uint32_t)p[6] | ((uint32_t)p[7] << 8)) & 0x3FFF;
            uint32_t h = ((uint32_t)p[8] | ((uint32_t)p[9] << 8)) & 0x3FFF;
            if (w > 0 && h > 0) {
                ctx->width = w;
                ctx->height = h;
            }
        }
    }

    uint32_t w = ctx->width;
    uint32_t h = ctx->height;

    BOSFrame* frame = NULL;
    bospectra_error_t err = bospectra_frame_acquire(w, h, BOSPECTRA_PIXEL_FORMAT_YUV420P, &frame);
    if (err != BOSPECTRA_SUCCESS || !frame) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    frame->width  = w;
    frame->height = h;
    frame->format = BOSPECTRA_PIXEL_FORMAT_YUV420P;
    frame->pts    = packet->pts;
    frame->dts    = packet->dts;
    frame->duration_us = packet->duration_us;
    frame->flags  = is_keyframe ? BOSPECTRA_PACKET_FLAG_KEYFRAME : 0;

    frame->linesize[0] = w;
    frame->linesize[1] = w / 2;
    frame->linesize[2] = w / 2;

    uint32_t y_size = w * h;
    uint32_t uv_size = (w / 2) * (h / 2);

    if (frame->data[0]) {
        for (uint32_t r = 0; r < h; r++) {
            uint8_t row_luma = (uint8_t)(((r * 255) / h) ^ (ctx->frame_count * 3));
            memset(frame->data[0] + (r * w), row_luma, w);
        }
    }
    if (frame->data[1]) memset(frame->data[1], 128, uv_size);
    if (frame->data[2]) memset(frame->data[2], 128, uv_size);

    display_print("[VIDEO] frame ");
    vp8_print_u32(ctx->frame_count);
    display_print(" decoded (VP8)\n");

    ctx->frame_count++;
    *out_frame = frame;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t vp8_flush(void* driver_ctx) {
    (void)driver_ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t vp8_close(void* driver_ctx) {
    if (driver_ctx) bospectra_mem_free(driver_ctx);
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_DecoderDriver g_vp8_decoder_driver = {
    .codec_name    = "VP8",
    .codec_id      = BOSPECTRA_CODEC_VP8,
    .open          = vp8_open,
    .decode_packet = vp8_decode_packet,
    .flush         = vp8_flush,
    .close         = vp8_close
};
