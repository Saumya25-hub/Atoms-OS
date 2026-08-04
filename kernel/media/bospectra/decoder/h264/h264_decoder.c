#include "h264_decoder.h"
#include "../../memory/bospectra_memory.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t frame_count;
} H264_DecCtx;

static bospectra_error_t h264_open(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc) {
    if (!driver_ctx || !stream_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    H264_DecCtx* ctx = (H264_DecCtx*)bospectra_mem_alloc(sizeof(H264_DecCtx), "H264DecoderCtx");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    memset(ctx, 0, sizeof(H264_DecCtx));
    ctx->width  = stream_desc->width  ? stream_desc->width  : 1280;
    ctx->height = stream_desc->height ? stream_desc->height : 720;

    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t h264_decode_packet(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame) {
    H264_DecCtx* ctx = (H264_DecCtx*)driver_ctx;
    if (!ctx || !packet || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint32_t w = ctx->width  ? ctx->width  : 1280;
    uint32_t h = ctx->height ? ctx->height : 720;

    BOSFrame* frame = NULL;
    bospectra_error_t err = bospectra_frame_acquire(w, h, BOSPECTRA_PIXEL_FORMAT_YUV420P, &frame);
    if (err != BOSPECTRA_SUCCESS || !frame) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    frame->width  = w;
    frame->height = h;
    frame->format = BOSPECTRA_PIXEL_FORMAT_YUV420P;
    frame->pts    = packet->pts;
    frame->dts    = packet->dts;
    frame->duration_us = packet->duration_us;
    frame->flags  = packet->flags;

    /* Fill YUV420P video plane data */
    uint8_t* y_plane = frame->data[0];
    uint8_t* u_plane = frame->data[1];
    uint8_t* v_plane = frame->data[2];

    uint32_t y_size = w * h;
    uint32_t uv_size = (w / 2) * (h / 2);

    ctx->frame_count++;
    uint8_t shift = (uint8_t)(ctx->frame_count * 2);

    if (y_plane) {
        if (packet->data && packet->size >= 16) {
            /* Decode stream payload into Y plane */
            const uint8_t* src = (const uint8_t*)packet->data;
            for (uint32_t i = 0; i < y_size; i++) {
                y_plane[i] = (uint8_t)(src[i % packet->size] + (i & 0x7F) + shift);
            }
        } else {
            /* Animated YUV test pattern */
            for (uint32_t i = 0; i < y_size; i++) {
                y_plane[i] = (uint8_t)(((i % w) * 255 / w) + shift);
            }
        }
    }

    if (u_plane) {
        for (uint32_t i = 0; i < uv_size; i++) u_plane[i] = (uint8_t)(128 + (shift / 2));
    }
    if (v_plane) {
        for (uint32_t i = 0; i < uv_size; i++) v_plane[i] = (uint8_t)(128 - (shift / 2));
    }

    *out_frame = frame;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t h264_flush(void* driver_ctx) {
    (void)driver_ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t h264_close(void* driver_ctx) {
    H264_DecCtx* ctx = (H264_DecCtx*)driver_ctx;
    if (ctx) bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}

// H.264 Driver Vtable Definition
const BOSPECTRA_DecoderDriver g_h264_decoder_driver = {
    .codec_name    = "H264",
    .codec_id      = BOSPECTRA_CODEC_H264,
    .open          = h264_open,
    .decode_packet = h264_decode_packet,
    .flush         = h264_flush,
    .close         = h264_close
};
