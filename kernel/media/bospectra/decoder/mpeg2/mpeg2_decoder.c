#include "mpeg2_decoder.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

static bospectra_error_t mpeg2_open(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc) {
    (void)stream_desc;
    if (!driver_ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    *driver_ctx = (void*)(uintptr_t)1; // Stub context handle
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mpeg2_decode_packet(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame) {
    (void)driver_ctx;
    if (!packet || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    BOSFrame* frame = NULL;
    bospectra_error_t err = bospectra_frame_acquire(1920, 1080, BOSPECTRA_PIXEL_FORMAT_YUV420P, &frame);
    if (err != BOSPECTRA_SUCCESS || !frame) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    frame->width = 1920;
    frame->height = 1080;
    frame->format = BOSPECTRA_PIXEL_FORMAT_YUV420P;
    frame->pts = packet->pts;

    *out_frame = frame;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mpeg2_flush(void* driver_ctx) {
    (void)driver_ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mpeg2_close(void* driver_ctx) {
    (void)driver_ctx;
    return BOSPECTRA_SUCCESS;
}

// MPEG2 Driver Vtable Definition
const BOSPECTRA_DecoderDriver g_mpeg2_decoder_driver = {
    .codec_name    = "MPEG2",
    .codec_id      = BOSPECTRA_CODEC_MPEG2,
    .open          = mpeg2_open,
    .decode_packet = mpeg2_decode_packet,
    .flush         = mpeg2_flush,
    .close         = mpeg2_close
};
