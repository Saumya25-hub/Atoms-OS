#include "h264_decoder.h"
#include "third_party/media/h264/include/h264bsd_decoder.h"
#include "third_party/media/mp4/include/mp4_demux.h"
#include "../../memory/bospectra_memory.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../include/bospectra_errors.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/atoms_debug_boot.h"

extern void display_print(const char* str);
extern void com1_puts(const char* s);

typedef struct {
    storage_t* storage;
    uint32_t   width;
    uint32_t   height;
    uint32_t   frame_count;
    bool       initialized;
    bool       headers_ready;
} H264_DecCtx;

static void print_u32(uint32_t val) {
    char buf[16];
    char rev[16];
    int r = 0;
    if (val == 0) {
        display_print("0");
        return;
    }
    while (val > 0) {
        rev[r++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = 0; i < r; i++) {
        buf[i] = rev[r - 1 - i];
    }
    buf[r] = '\0';
    display_print(buf);
    com1_puts(buf);
}

static void h264_print_hex32(uint32_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    char buf[9];
    for (int i = 7; i >= 0; i--) {
        buf[7 - i] = hex_chars[(val >> (i * 4)) & 0xF];
    }
    buf[8] = '\0';
    com1_puts(buf);
}

static bospectra_error_t h264_open(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc) {
    if (!driver_ctx || !stream_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    H264_DecCtx* ctx = (H264_DecCtx*)bospectra_mem_alloc(sizeof(H264_DecCtx), "H264DecoderCtx");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;
    memset(ctx, 0, sizeof(H264_DecCtx));

    ctx->width  = stream_desc->width  ? stream_desc->width  : 640;
    ctx->height = stream_desc->height ? stream_desc->height : 360;

    ctx->storage = h264bsdAlloc();
    if (!ctx->storage) {
        bospectra_mem_free(ctx);
        return BOSPECTRA_ERR_OUT_OF_MEMORY;
    }

    u32 init_ret = h264bsdInit(ctx->storage, 1);
    if (init_ret != 0) {
        com1_puts("[MEDIA_DEBUG] H264 INIT: FAIL\r\n");
        atoms_first_failure_record("H264_INIT");
        h264bsdFree(ctx->storage);
        bospectra_mem_free(ctx);
        return BOSPECTRA_ERR_SUBSYSTEM_FAILED;
    }
    com1_puts("[MEDIA_DEBUG] H264 INIT: PASS\r\n");

    ctx->initialized = true;

    /* Feed extradata (SPS and PPS) if provided by demuxer */
    if (stream_desc->extradata && stream_desc->extradata_size >= sizeof(MP4_DemuxTrack)) {
        const MP4_DemuxTrack* trk = (const MP4_DemuxTrack*)stream_desc->extradata;
        u32 read_bytes = 0;

        if (trk->sps_len > 0) {
            uint8_t sps_annexb[MP4_MAX_SPS_LEN + 4];
            sps_annexb[0] = 0; sps_annexb[1] = 0; sps_annexb[2] = 0; sps_annexb[3] = 1;
            memcpy(sps_annexb + 4, trk->sps, trk->sps_len);
            u32 ret = h264bsdDecode(ctx->storage, sps_annexb, 4 + trk->sps_len, 0, &read_bytes);
            if (ret == H264BSD_RDY || ret == H264BSD_HDRS_RDY) {
                display_print("[H264] SPS = OK\n");
            }
        }

        if (trk->pps_len > 0) {
            uint8_t pps_annexb[MP4_MAX_PPS_LEN + 4];
            pps_annexb[0] = 0; pps_annexb[1] = 0; pps_annexb[2] = 0; pps_annexb[3] = 1;
            memcpy(pps_annexb + 4, trk->pps, trk->pps_len);
            u32 ret = h264bsdDecode(ctx->storage, pps_annexb, 4 + trk->pps_len, 0, &read_bytes);
            if (ret == H264BSD_RDY || ret == H264BSD_HDRS_RDY) {
                display_print("[H264] PPS = OK\n");
            }
        }
    }

    display_print("[H264] decoder initialized\n");

    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t h264_decode_packet(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame) {
    display_print("[H264] decode_packet enter\n");
    H264_DecCtx* ctx = (H264_DecCtx*)driver_ctx;
    if (!ctx || !packet || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (!ctx->storage) return BOSPECTRA_ERR_NOT_INITIALIZED;

    if (!packet->data || packet->size == 0) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    static uint32_t s_packet_counter = 0;
    s_packet_counter++;

    if (s_packet_counter <= 10 || (s_packet_counter % 30 == 0)) {
        com1_puts("[MEDIA_DEBUG] PACKET #: ");
        print_u32(s_packet_counter);
        com1_puts("\r\n");

        uint8_t nal_type = 0;
        if (packet->size >= 5) {
            nal_type = packet->data[4] & 0x1F;
        }
        com1_puts("[MEDIA_DEBUG] NAL: ");
        print_u32(nal_type);
        com1_puts("\r\n");
    }

    /* Convert AVCC 4-byte length prefixes to Annex B start codes (00 00 00 01) */
    uint8_t* p = (uint8_t*)packet->data;
    uint32_t offset = 0;

    while (offset + 4 <= packet->size) {
        uint32_t nal_len = ((uint32_t)p[offset] << 24) | ((uint32_t)p[offset + 1] << 16) |
                           ((uint32_t)p[offset + 2] << 8)  | (uint32_t)p[offset + 3];
        if (nal_len == 0 || offset + 4 + nal_len > packet->size) {
            if (p[offset] == 0 && p[offset+1] == 0 && (p[offset+2] == 1 || (p[offset+2] == 0 && p[offset+3] == 1))) {
                break;
            }
            break;
        }
        p[offset]     = 0x00;
        p[offset + 1] = 0x00;
        p[offset + 2] = 0x00;
        p[offset + 3] = 0x01;
        offset += 4 + nal_len;
    }

    /* Feed NAL units to h264bsd */
    uint8_t* cur = (uint8_t*)packet->data;
    uint32_t rem = packet->size;
    u32 read_bytes = 0;
    uint32_t loop_guard = 0;

    while (rem > 0 && ++loop_guard < 1000) {
        u32 ret = h264bsdDecode(ctx->storage, cur, rem, ctx->frame_count, &read_bytes);
        if (ret == H264BSD_HDRS_RDY) {
            ctx->headers_ready = true;
            u32 pw = h264bsdPicWidth(ctx->storage) * 16;
            u32 ph = h264bsdPicHeight(ctx->storage) * 16;
            if (pw > 0) ctx->width = pw;
            if (ph > 0) ctx->height = ph;
            /* In h264bsd, HDRS_RDY returns readBytes=0 and sets prevBufNotFinished=1.
             * Continue with same buffer pointer to decode the pending slice. */
            continue;
        } else if (ret == H264BSD_ERROR || ret == H264BSD_PARAM_SET_ERROR) {
            if (read_bytes == 0) read_bytes = 1;
        }

        if (read_bytes == 0 || read_bytes > rem) break;
        cur += read_bytes;
        rem -= read_bytes;
    }

    /* Retrieve decoded frame */
    u32 pic_id = 0, is_idr = 0, num_err = 0;
    u8* yuv_data = h264bsdNextOutputPicture(ctx->storage, &pic_id, &is_idr, &num_err);

    if (yuv_data) {
        u32 w = h264bsdPicWidth(ctx->storage) * 16;
        u32 h = h264bsdPicHeight(ctx->storage) * 16;
        u32 crop_flag = 0, crop_left = 0, crop_w = 0, crop_top = 0, crop_h = 0;
        h264bsdCroppingParams(ctx->storage, &crop_flag, &crop_left, &crop_w, &crop_top, &crop_h);
        u32 visible_w = (crop_flag && crop_w > 0) ? crop_w : w;
        u32 visible_h = (crop_flag && crop_h > 0) ? crop_h : h;

        BOSFrame* frame = NULL;
        bospectra_error_t err = bospectra_frame_acquire(w, h, BOSPECTRA_PIXEL_FORMAT_YUV420P, &frame);
        if (err != BOSPECTRA_SUCCESS || !frame) return BOSPECTRA_ERR_OUT_OF_MEMORY;

        frame->width  = visible_w;
        frame->height = visible_h;
        frame->format = BOSPECTRA_PIXEL_FORMAT_YUV420P;
        frame->pts    = packet->pts;
        frame->dts    = packet->dts;
        frame->duration_us = packet->duration_us;
        frame->flags  = is_idr ? BOSPECTRA_PACKET_FLAG_KEYFRAME : 0;

        frame->linesize[0] = w;
        frame->linesize[1] = w / 2;
        frame->linesize[2] = w / 2;

        /* Planar YUV420P copy */
        uint32_t y_size = w * h;
        uint32_t uv_size = (w / 2) * (h / 2);

        if (frame->data[0]) memcpy(frame->data[0], yuv_data, y_size);
        if (frame->data[1]) memcpy(frame->data[1], yuv_data + y_size, uv_size);
        if (frame->data[2]) memcpy(frame->data[2], yuv_data + y_size + uv_size, uv_size);

        display_print("[VIDEO] frame ");
        print_u32(ctx->frame_count);
        display_print(" decoded\n");

        ctx->frame_count++;
        *out_frame = frame;

        if (ctx->frame_count <= ATOMS_MEDIA_DEBUG_FRAMES) {
            com1_puts("[MEDIA_DEBUG] DECODE RESULT: PASS\r\n");
            com1_puts("[MEDIA_DEBUG] FRAME OUTPUT: PASS\r\n");

            uint32_t total_yuv_size = y_size + uv_size * 2;
            uint32_t dec_crc = atoms_crc32(yuv_data, total_yuv_size);
            com1_puts("[FRAME_DEBUG] frame=");
            print_u32(ctx->frame_count - 1);
            com1_puts("\r\n");
            com1_puts("[FRAME_DEBUG] decoded_crc=0x");
            h264_print_hex32(dec_crc);
            com1_puts("\r\n");
        }

        return BOSPECTRA_SUCCESS;
    }

    if (ctx->frame_count == 0 && s_packet_counter > 5) {
        if (!atoms_first_failure_occurred()) {
            com1_puts("[MEDIA_DEBUG] DECODE RESULT: FAIL\r\n");
            com1_puts("[MEDIA_DEBUG] FRAME OUTPUT: FAIL\r\n");
            atoms_first_failure_record("H264_FRAME_OUTPUT");
        }
    }

    display_print("[H264] no yuv_data\n");
    *out_frame = NULL;
    return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
}

static bospectra_error_t h264_flush(void* driver_ctx) {
    H264_DecCtx* ctx = (H264_DecCtx*)driver_ctx;
    if (!ctx || !ctx->storage) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    h264bsdFlushBuffer(ctx->storage);
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t h264_close(void* driver_ctx) {
    H264_DecCtx* ctx = (H264_DecCtx*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (ctx->storage) {
        h264bsdShutdown(ctx->storage);
        h264bsdFree(ctx->storage);
        ctx->storage = NULL;
    }
    bospectra_mem_free(ctx);
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
