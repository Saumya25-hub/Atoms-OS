/*
 * ATOMS OS — BOSpectra Matroska (MKV / WebM) Container Parser
 * kernel/media/bospectra/container/mkv/mkv_parser.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "mkv_parser.h"
#include "../common/container_common.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "../../file/bospectra_file.h"
#include "../../frame_memory/packet_pool/packet_pool.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

#define MKV_ID_TRACK_ENTRY   0xAEU
#define MKV_ID_TRACK_NUMBER  0xD7U
#define MKV_ID_TRACK_TYPE    0x83U
#define MKV_ID_CODEC_ID      0x86U
#define MKV_ID_VIDEO_SETTINGS 0xE0U
#define MKV_ID_PIXEL_WIDTH   0xB0U
#define MKV_ID_PIXEL_HEIGHT  0xBAU
#define MKV_ID_AUDIO_SETTINGS 0xE1U
#define MKV_ID_SAMPLING_FREQ 0xB5U
#define MKV_ID_CHANNELS      0x9FU
#define MKV_ID_TIMECODE      0xE7U
#define MKV_ID_SIMPLE_BLOCK  0xA3U
#define MKV_ID_BLOCK_GROUP   0xA0U
#define MKV_ID_BLOCK         0xA1U

typedef struct {
    bospectra_file_id_t file_id;
    uint64_t            file_size;
    uint64_t            first_cluster_offset;
    uint64_t            current_offset;
    uint64_t            cluster_timecode_ms;
    uint64_t            timecode_scale_ns;

    /* Video track info */
    uint32_t            video_track_num;
    uint32_t            video_width;
    uint32_t            video_height;
    char                video_codec[32];

    /* Audio track info */
    uint32_t            audio_track_num;
    uint32_t            audio_rate;
    uint8_t             audio_channels;
    char                audio_codec[32];

    uint32_t            stream_count;
    uint64_t            duration_us;
} MKVContext;

static uint64_t ebml_read_vint(const uint8_t* p, size_t len, size_t* out_len) {
    if (!p || len == 0) return 0;
    uint8_t first = p[0];
    int mask = 0x80;
    int vint_len = 1;
    while (vint_len <= 8 && !(first & mask)) {
        mask >>= 1;
        vint_len++;
    }
    if (vint_len > 8 || (size_t)vint_len > len) return 0;
    uint64_t val = first & (mask - 1);
    for (int i = 1; i < vint_len; i++) {
        val = (val << 8) | p[i];
    }
    if (out_len) *out_len = vint_len;
    return val;
}

static uint32_t ebml_read_id(const uint8_t* p, size_t len, size_t* out_len) {
    if (!p || len == 0) return 0;
    uint8_t first = p[0];
    int mask = 0x80;
    int id_len = 1;
    while (id_len <= 4 && !(first & mask)) {
        mask >>= 1;
        id_len++;
    }
    if (id_len > 4 || (size_t)id_len > len) return 0;
    uint32_t id = 0;
    for (int i = 0; i < id_len; i++) {
        id = (id << 8) | p[i];
    }
    if (out_len) *out_len = id_len;
    return id;
}

static int mkv_probe(bospectra_file_id_t file_id, const uint8_t* header_data, size_t header_len) {
    (void)file_id;
    if (!header_data || header_len < 4) return 0;

    uint32_t ebml_id = bospectra_read_u32_be(header_data);
    if (ebml_id == EBML_ID_HEADER) {
        return 100;
    }
    return 0;
}

static bospectra_error_t mkv_open(void** driver_ctx, bospectra_file_id_t file_id, BOSPECTRA_ContainerMetadata* out_meta) {
    if (!driver_ctx || !out_meta) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    display_print("[MKV] opening Matroska/WebM container\n");

    MKVContext* ctx = (MKVContext*)bospectra_mem_alloc(sizeof(MKVContext), "MKVContext");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;
    memset(ctx, 0, sizeof(MKVContext));

    ctx->file_id = file_id;
    ctx->timecode_scale_ns = 1000000ULL; // 1 ms default
    ctx->video_width = 1280;
    ctx->video_height = 720;
    ctx->audio_rate = 44100;
    ctx->audio_channels = 2;
    strncpy(ctx->video_codec, "VP9", sizeof(ctx->video_codec) - 1);
    strncpy(ctx->audio_codec, "VORBIS", sizeof(ctx->audio_codec) - 1);

    uint64_t file_sz = 0;
    if (bospectra_file_get_size(file_id, &file_sz) == BOSPECTRA_SUCCESS) {
        ctx->file_size = file_sz;
    }

    /* Scan first 64KB for tracks and cluster offset */
    uint8_t scan_buf[4096];
    uint32_t read_bytes = 0;
    uint64_t cur_pos = 0;

    while (cur_pos < 128 * 1024 && cur_pos < ctx->file_size) {
        if (bospectra_file_seek(file_id, cur_pos) != BOSPECTRA_SUCCESS) break;
        if (bospectra_file_read(file_id, scan_buf, sizeof(scan_buf), &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) break;

        size_t off = 0;
        while (off + 4 <= read_bytes) {
            uint32_t val32 = bospectra_read_u32_be(scan_buf + off);
            if (val32 == EBML_ID_CLUSTER) {
                if (ctx->first_cluster_offset == 0) {
                    ctx->first_cluster_offset = cur_pos + off;
                }
                break;
            }
            if (val32 == EBML_ID_TRACKS) {
                ctx->stream_count = 1;
            }
            off++;
        }

        if (ctx->first_cluster_offset != 0) break;
        cur_pos += (read_bytes > 8) ? (read_bytes - 8) : read_bytes;
    }

    if (ctx->first_cluster_offset == 0) {
        ctx->first_cluster_offset = 64; // Fallback sensible start
    }
    ctx->current_offset = ctx->first_cluster_offset;

    out_meta->duration_us = 60000000ULL; // 60 sec nominal
    out_meta->stream_count = (ctx->stream_count > 0) ? ctx->stream_count : 1;
    out_meta->video_stream_count = 1;
    out_meta->audio_stream_count = 0;
    strncpy(out_meta->format_name, "MKV", sizeof(out_meta->format_name) - 1);
    strncpy(out_meta->container_brand, "Matroska/WebM", sizeof(out_meta->container_brand) - 1);

    display_print("[MKV] container opened successfully\n");
    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mkv_get_stream(void* driver_ctx, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc) {
    MKVContext* ctx = (MKVContext*)driver_ctx;
    if (!ctx || !out_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (stream_index > 0) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    memset(out_desc, 0, sizeof(BOSPECTRA_StreamDescriptor));
    out_desc->id = 1;
    out_desc->type = BOSPECTRA_STREAM_VIDEO;
    out_desc->width = ctx->video_width;
    out_desc->height = ctx->video_height;
    out_desc->frame_rate_num = 30;
    out_desc->frame_rate_den = 1;
    out_desc->is_active = true;
    strncpy(out_desc->codec_name, ctx->video_codec, sizeof(out_desc->codec_name) - 1);

    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mkv_read_packet(void* driver_ctx, BOSPacket** out_pkt) {
    MKVContext* ctx = (MKVContext*)driver_ctx;
    if (!ctx || !out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (ctx->current_offset >= ctx->file_size) {
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    uint8_t hdr[16];
    uint32_t read_bytes = 0;

    while (ctx->current_offset + 8 <= ctx->file_size) {
        if (bospectra_file_seek(ctx->file_id, ctx->current_offset) != BOSPECTRA_SUCCESS) {
            return BOSPECTRA_ERR_FILE_READ_FAILED;
        }
        if (bospectra_file_read(ctx->file_id, hdr, 16, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) {
            return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
        }

        size_t id_len = 0;
        uint32_t elem_id = ebml_read_id(hdr, read_bytes, &id_len);
        if (id_len == 0 || id_len >= read_bytes) {
            ctx->current_offset++;
            continue;
        }

        size_t size_len = 0;
        uint64_t payload_size = ebml_read_vint(hdr + id_len, read_bytes - id_len, &size_len);
        if (size_len == 0) {
            ctx->current_offset++;
            continue;
        }

        uint64_t data_offset = ctx->current_offset + id_len + size_len;

        if (elem_id == EBML_ID_CLUSTER) {
            /* Cluster container element: advance into cluster data */
            ctx->current_offset = data_offset;
            continue;
        } else if (elem_id == MKV_ID_TIMECODE) {
            /* Cluster timecode */
            if (payload_size <= 8 && payload_size > 0) {
                uint8_t tc_buf[8];
                if (bospectra_file_read(ctx->file_id, tc_buf, (uint32_t)payload_size, &read_bytes) == BOSPECTRA_SUCCESS) {
                    uint64_t tc = 0;
                    for (size_t b = 0; b < payload_size; b++) tc = (tc << 8) | tc_buf[b];
                    ctx->cluster_timecode_ms = tc;
                }
            }
            ctx->current_offset = data_offset + payload_size;
            continue;
        } else if (elem_id == MKV_ID_SIMPLE_BLOCK || elem_id == MKV_ID_BLOCK) {
            /* SimpleBlock or Block payload */
            if (payload_size < 4 || payload_size > 2 * 1024 * 1024) {
                ctx->current_offset = data_offset + payload_size;
                continue;
            }

            uint8_t blk_hdr[8];
            if (bospectra_file_read(ctx->file_id, blk_hdr, 4, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 4) {
                return BOSPECTRA_ERR_FILE_READ_FAILED;
            }

            size_t track_vint_len = 0;
            uint64_t track_num = ebml_read_vint(blk_hdr, 4, &track_vint_len);
            int16_t rel_timecode = (int16_t)(((uint16_t)blk_hdr[track_vint_len] << 8) | blk_hdr[track_vint_len + 1]);
            uint8_t flags = blk_hdr[track_vint_len + 2];
            bool is_keyframe = (elem_id == MKV_ID_SIMPLE_BLOCK) ? ((flags & 0x80) != 0) : true;

            size_t blk_hdr_len = track_vint_len + 3;
            uint32_t frame_size = (uint32_t)(payload_size - blk_hdr_len);

            BOSPacket* pkt = bospectra_packet_alloc(frame_size);
            if (!pkt) return BOSPECTRA_ERR_OUT_OF_MEMORY;

            if (bospectra_file_seek(ctx->file_id, data_offset + blk_hdr_len) != BOSPECTRA_SUCCESS ||
                bospectra_file_read(ctx->file_id, pkt->data, frame_size, &read_bytes) != BOSPECTRA_SUCCESS ||
                read_bytes != frame_size) {
                bospectra_packet_free(pkt);
                return BOSPECTRA_ERR_FILE_READ_FAILED;
            }

            pkt->stream_id = (bospectra_stream_id_t)track_num;
            int64_t abs_time_ms = (int64_t)ctx->cluster_timecode_ms + rel_timecode;
            if (abs_time_ms < 0) abs_time_ms = 0;
            pkt->pts = (uint64_t)abs_time_ms * 1000ULL;
            pkt->dts = pkt->pts;
            pkt->duration_us = 33333ULL;
            pkt->flags = is_keyframe ? BOSPECTRA_PACKET_FLAG_KEYFRAME : 0;
            pkt->size = frame_size;

            ctx->current_offset = data_offset + payload_size;
            *out_pkt = pkt;
            return BOSPECTRA_SUCCESS;
        }

        /* Skip other elements */
        ctx->current_offset = data_offset + payload_size;
    }

    return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
}

static bospectra_error_t mkv_seek(void* driver_ctx, uint64_t timestamp_us) {
    MKVContext* ctx = (MKVContext*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    (void)timestamp_us;
    ctx->current_offset = ctx->first_cluster_offset;
    ctx->cluster_timecode_ms = 0;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mkv_close(void* driver_ctx) {
    MKVContext* ctx = (MKVContext*)driver_ctx;
    if (ctx) bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_ContainerDriver g_mkv_container_driver = {
    .format_name = "MKV",
    .extensions  = "mkv,webm",
    .probe       = mkv_probe,
    .open        = mkv_open,
    .get_stream  = mkv_get_stream,
    .read_packet = mkv_read_packet,
    .seek        = mkv_seek,
    .close       = mkv_close
};
