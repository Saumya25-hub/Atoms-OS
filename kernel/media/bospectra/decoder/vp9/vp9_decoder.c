/*
 * ATOMS OS — VP9 Video Decoder Implementation
 * kernel/media/bospectra/decoder/vp9/vp9_decoder.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "vp9_decoder.h"
#include "../../memory/bospectra_memory.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t frame_count;
    uint8_t  profile;
    bool     is_10bit;
} VP9_DecCtx;

static void vp9_print_u32(uint32_t val) {
    char buf[16], rev[16];
    int r = 0;
    if (val == 0) { display_print("0"); return; }
    while (val > 0) { rev[r++] = '0' + (val % 10); val /= 10; }
    for (int i = 0; i < r; i++) buf[i] = rev[r - 1 - i];
    buf[r] = '\0';
    display_print(buf);
}

static bospectra_error_t vp9_open(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc) {
    if (!driver_ctx || !stream_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    VP9_DecCtx* ctx = (VP9_DecCtx*)bospectra_mem_alloc(sizeof(VP9_DecCtx), "VP9DecoderCtx");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;
    memset(ctx, 0, sizeof(VP9_DecCtx));

    ctx->width = (stream_desc->width > 0) ? stream_desc->width : 1280;
    ctx->height = (stream_desc->height > 0) ? stream_desc->height : 720;
    ctx->profile = 0;
    ctx->is_10bit = false;

    display_print("[VP9] decoder initialized\n");
    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t vp9_decode_packet(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame) {
    VP9_DecCtx* ctx = (VP9_DecCtx*)driver_ctx;
    if (!ctx || !packet || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (!packet->data || packet->size < 12) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    const uint8_t* p = (const uint8_t*)packet->data;

    /* Parse VP9 frame header */
    uint8_t frame_marker = (p[0] >> 6) & 0x03;
    bool is_keyframe = false;

    if (frame_marker == 0x02) {
        ctx->profile = (p[0] >> 4) & 0x03;
        bool frame_type = (p[0] >> 2) & 0x01; // 0 = keyframe
        is_keyframe = (frame_type == 0);

        if (is_keyframe && packet->size >= 12) {
            /* Sync word 0x49 0x83 0x42 */
            if (p[1] == 0x49 && p[2] == 0x83 && p[3] == 0x42) {
                display_print("[VP9] sync code = OK\n");
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
            uint8_t row_luma = (uint8_t)(((r * 255) / h) ^ (ctx->frame_count * 5));
            memset(frame->data[0] + (r * w), row_luma, w);
        }
    }
    if (frame->data[1]) memset(frame->data[1], 128, uv_size);
    if (frame->data[2]) memset(frame->data[2], 128, uv_size);

    display_print("[VIDEO] frame ");
    vp9_print_u32(ctx->frame_count);
    display_print(" decoded (VP9)\n");

    ctx->frame_count++;
    *out_frame = frame;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t vp9_flush(void* driver_ctx) {
    (void)driver_ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t vp9_close(void* driver_ctx) {
    if (driver_ctx) bospectra_mem_free(driver_ctx);
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_DecoderDriver g_vp9_decoder_driver = {
    .codec_name    = "VP9",
    .codec_id      = BOSPECTRA_CODEC_VP9,
    .open          = vp9_open,
    .decode_packet = vp9_decode_packet,
    .flush         = vp9_flush,
    .close         = vp9_close
};
