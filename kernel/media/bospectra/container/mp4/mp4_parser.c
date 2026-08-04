#include "mp4_parser.h"
#include "../common/container_common.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

#define MP4_MAX_TRACKS 8U

typedef struct {
    uint32_t track_id;
    bospectra_stream_type_t type;
    char     codec_name[32];
    uint32_t timescale;
    uint64_t duration_us;
    uint32_t width;
    uint32_t height;
    uint32_t sample_rate;
    uint8_t  channels;
    
    // Sample table metadata offsets
    uint64_t stsz_offset;
    uint32_t sample_count;
    uint64_t stco_offset;
    uint32_t chunk_count;
    uint32_t current_sample_idx;
} MP4_TrackContext;

typedef struct {
    bospectra_file_id_t file_id;
    uint64_t            file_size;
    uint32_t            timescale;
    uint64_t            duration_us;
    char                major_brand[8];
    MP4_TrackContext    tracks[MP4_MAX_TRACKS];
    uint32_t            track_count;
    uint32_t            active_track_idx;
} MP4_ParserContext;

// MP4 Driver Probing
static int mp4_probe(bospectra_file_id_t file_id, const uint8_t* header_data, size_t header_len) {
    (void)file_id;
    if (!header_data || header_len < 12) return 0;

    uint32_t box_size = bospectra_read_u32_be(header_data);
    uint32_t box_type = bospectra_read_u32_be(header_data + 4);

    if (box_type == MP4_BOX_FTYP && box_size >= 12 && box_size <= header_len) {
        uint32_t brand = bospectra_read_u32_be(header_data + 8);
        if (brand == BOSPECTRA_FOURCC('i', 's', 'o', 'm') ||
            brand == BOSPECTRA_FOURCC('m', 'p', '4', '2') ||
            brand == BOSPECTRA_FOURCC('i', 's', 'o', '2') ||
            brand == BOSPECTRA_FOURCC('a', 'v', 'c', '1') ||
            brand == BOSPECTRA_FOURCC('M', 'P', '4', ' ')) {
            return 100;
        }
        return 80; // Valid ftyp box
    }

    return 0;
}

// MP4 Box Traversal & Metadata Parsing
static bospectra_error_t mp4_parse_boxes(MP4_ParserContext* ctx, uint64_t offset, uint64_t max_offset) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint8_t box_hdr[16];
    uint32_t bytes_read = 0;

    while (offset + 8 <= max_offset) {
        if (bospectra_file_seek(ctx->file_id, offset) != BOSPECTRA_SUCCESS) break;
        if (bospectra_file_read(ctx->file_id, box_hdr, 8, &bytes_read) != BOSPECTRA_SUCCESS || bytes_read < 8) break;

        uint64_t box_size = bospectra_read_u32_be(box_hdr);
        uint32_t box_type = bospectra_read_u32_be(box_hdr + 4);
        uint64_t header_size = 8;

        if (box_size == 1) { // 64-bit extended box size
            if (bospectra_file_read(ctx->file_id, box_hdr + 8, 8, &bytes_read) != BOSPECTRA_SUCCESS || bytes_read < 8) break;
            box_size = bospectra_read_u64_be(box_hdr + 8);
            header_size = 16;
        } else if (box_size == 0) { // Box extends to EOF
            box_size = max_offset - offset;
        }

        if (box_size < header_size || offset + box_size > max_offset) break; // Security boundary guard

        if (box_type == MP4_BOX_FTYP && box_size >= 12) {
            uint8_t brand_buf[4];
            if (bospectra_file_read(ctx->file_id, brand_buf, 4, &bytes_read) == BOSPECTRA_SUCCESS && bytes_read == 4) {
                memcpy(ctx->major_brand, brand_buf, 4);
                ctx->major_brand[4] = '\0';
            }
        } else if (box_type == MP4_BOX_MOOV) {
            // Recursively parse moov container box
            mp4_parse_boxes(ctx, offset + header_size, offset + box_size);
        } else if (box_type == MP4_BOX_MVHD && box_size >= header_size + 20) {
            uint8_t mvhd_buf[32];
            if (bospectra_file_read(ctx->file_id, mvhd_buf, 24, &bytes_read) == BOSPECTRA_SUCCESS && bytes_read >= 24) {
                uint8_t version = mvhd_buf[0];
                if (version == 1 && box_size >= header_size + 32) {
                    ctx->timescale = bospectra_read_u32_be(mvhd_buf + 20);
                    ctx->duration_us = (bospectra_read_u64_be(mvhd_buf + 24) * 1000000ULL) / (ctx->timescale ? ctx->timescale : 1);
                } else if (version == 0) {
                    ctx->timescale = bospectra_read_u32_be(mvhd_buf + 12);
                    uint32_t dur = bospectra_read_u32_be(mvhd_buf + 16);
                    ctx->duration_us = ((uint64_t)dur * 1000000ULL) / (ctx->timescale ? ctx->timescale : 1);
                }
            }
        } else if (box_type == MP4_BOX_TRAK) {
            if (ctx->track_count < MP4_MAX_TRACKS) {
                MP4_TrackContext* trk = &ctx->tracks[ctx->track_count++];
                memset(trk, 0, sizeof(MP4_TrackContext));
                trk->track_id = ctx->track_count;
                trk->type = BOSPECTRA_STREAM_VIDEO; // Default fallback
                mp4_parse_boxes(ctx, offset + header_size, offset + box_size);
            }
        } else if (box_type == MP4_BOX_TKHD && box_size >= header_size + 80 && ctx->track_count > 0) {
            MP4_TrackContext* trk = &ctx->tracks[ctx->track_count - 1];
            uint8_t tkhd_buf[84];
            if (bospectra_file_read(ctx->file_id, tkhd_buf, 84, &bytes_read) == BOSPECTRA_SUCCESS && bytes_read >= 84) {
                trk->width = bospectra_read_u32_be(tkhd_buf + 76) >> 16;
                trk->height = bospectra_read_u32_be(tkhd_buf + 80) >> 16;
            }
        } else if (box_type == MP4_BOX_HDLR && box_size >= header_size + 12 && ctx->track_count > 0) {
            MP4_TrackContext* trk = &ctx->tracks[ctx->track_count - 1];
            uint8_t hdlr_buf[12];
            if (bospectra_file_read(ctx->file_id, hdlr_buf, 12, &bytes_read) == BOSPECTRA_SUCCESS && bytes_read >= 12) {
                uint32_t handler_type = bospectra_read_u32_be(hdlr_buf + 8);
                if (handler_type == BOSPECTRA_FOURCC('v', 'i', 'd', 'e')) {
                    trk->type = BOSPECTRA_STREAM_VIDEO;
                } else if (handler_type == BOSPECTRA_FOURCC('s', 'o', 'u', 'n')) {
                    trk->type = BOSPECTRA_STREAM_AUDIO;
                } else if (handler_type == BOSPECTRA_FOURCC('s', 'u', 'b', 't') || handler_type == BOSPECTRA_FOURCC('t', 'e', 'x', 't')) {
                    trk->type = BOSPECTRA_STREAM_SUBTITLE;
                }
            }
        } else if (box_type == MP4_BOX_STBL && ctx->track_count > 0) {
            mp4_parse_boxes(ctx, offset + header_size, offset + box_size);
        } else if (box_type == MP4_BOX_STSZ && box_size >= header_size + 12 && ctx->track_count > 0) {
            MP4_TrackContext* trk = &ctx->tracks[ctx->track_count - 1];
            uint8_t stsz_buf[12];
            if (bospectra_file_read(ctx->file_id, stsz_buf, 12, &bytes_read) == BOSPECTRA_SUCCESS && bytes_read >= 12) {
                trk->sample_count = bospectra_read_u32_be(stsz_buf + 8);
                trk->stsz_offset = offset + header_size + 12;
            }
        }

        offset += box_size;
    }

    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Open
static bospectra_error_t mp4_open(void** driver_ctx, bospectra_file_id_t file_id, BOSPECTRA_ContainerMetadata* out_meta) {
    if (!driver_ctx || !out_meta) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    MP4_ParserContext* ctx = (MP4_ParserContext*)bospectra_mem_alloc(sizeof(MP4_ParserContext), "MP4ParserContext");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    memset(ctx, 0, sizeof(MP4_ParserContext));
    ctx->file_id = file_id;
    if (bospectra_file_get_size(file_id, &ctx->file_size) != BOSPECTRA_SUCCESS || ctx->file_size == 0) {
        ctx->file_size = 1024 * 1024 * 100;
    }

    mp4_parse_boxes(ctx, 0, ctx->file_size);

    out_meta->duration_us = ctx->duration_us;
    out_meta->stream_count = ctx->track_count;
    strncpy(out_meta->format_name, "MP4", sizeof(out_meta->format_name) - 1);
    strncpy(out_meta->container_brand, ctx->major_brand[0] ? ctx->major_brand : "isom", sizeof(out_meta->container_brand) - 1);

    for (uint32_t i = 0; i < ctx->track_count; i++) {
        if (ctx->tracks[i].type == BOSPECTRA_STREAM_VIDEO) out_meta->video_stream_count++;
        else if (ctx->tracks[i].type == BOSPECTRA_STREAM_AUDIO) out_meta->audio_stream_count++;
        else if (ctx->tracks[i].type == BOSPECTRA_STREAM_SUBTITLE) out_meta->subtitle_stream_count++;
    }

    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Get Stream
static bospectra_error_t mp4_get_stream(void* driver_ctx, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx || !out_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (stream_index >= ctx->track_count) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    MP4_TrackContext* trk = &ctx->tracks[stream_index];
    memset(out_desc, 0, sizeof(BOSPECTRA_StreamDescriptor));

    out_desc->id = trk->track_id;
    out_desc->type = trk->type;
    out_desc->width = trk->width;
    out_desc->height = trk->height;
    out_desc->frame_rate_num = 30; // Default metadata fallback
    out_desc->frame_rate_den = 1;
    out_desc->sample_rate = 44100;
    out_desc->channels = 2;
    out_desc->is_active = true;
    strncpy(out_desc->codec_name, (trk->type == BOSPECTRA_STREAM_VIDEO) ? "H264" : "AAC", sizeof(out_desc->codec_name) - 1);

    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Read Packet
static bospectra_error_t mp4_read_packet(void* driver_ctx, BOSPacket** out_pkt) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx || !out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (ctx->track_count == 0) return BOSPECTRA_ERR_BUFFER_UNDERFLOW;

    MP4_TrackContext* trk = &ctx->tracks[ctx->active_track_idx];
    if (trk->current_sample_idx >= trk->sample_count && trk->sample_count > 0) {
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW; // EOF for track
    }

    size_t sample_size = 4096; // Simulated/demuxed sample payload size
    BOSPacket* pkt = bospectra_packet_alloc(sample_size);
    if (!pkt) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    pkt->stream_id = trk->track_id;
    pkt->pts = (trk->current_sample_idx * 33333ULL); // ~30 FPS microsecond calculation
    pkt->dts = pkt->pts;
    pkt->duration_us = 33333ULL;
    pkt->flags = (trk->current_sample_idx % 30 == 0) ? BOSPECTRA_PACKET_FLAG_KEYFRAME : 0;

    trk->current_sample_idx++;
    ctx->active_track_idx = (ctx->active_track_idx + 1) % ctx->track_count; // Round-robin stream demux

    *out_pkt = pkt;
    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Seek
static bospectra_error_t mp4_seek(void* driver_ctx, uint64_t timestamp_us) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < ctx->track_count; i++) {
        uint64_t sample_idx = timestamp_us / 33333ULL;
        if (sample_idx > ctx->tracks[i].sample_count) {
            sample_idx = ctx->tracks[i].sample_count;
        }
        ctx->tracks[i].current_sample_idx = (uint32_t)sample_idx;
    }

    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Close
static bospectra_error_t mp4_close(void* driver_ctx) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Vtable Definition
const BOSPECTRA_ContainerDriver g_mp4_container_driver = {
    .format_name = "MP4",
    .extensions  = "mp4,m4v,mov",
    .probe       = mp4_probe,
    .open        = mp4_open,
    .get_stream  = mp4_get_stream,
    .read_packet = mp4_read_packet,
    .seek        = mp4_seek,
    .close       = mp4_close
};
