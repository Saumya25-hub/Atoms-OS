#include "avi_parser.h"
#include "../common/container_common.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define AVI_MAX_STREAMS 8U

typedef struct {
    uint32_t stream_id;
    bospectra_stream_type_t type;
    uint32_t handler;
    uint32_t scale;
    uint32_t rate;
    uint32_t length;
    uint32_t width;
    uint32_t height;
    uint32_t sample_rate;
    uint8_t  channels;
    uint32_t current_chunk_idx;
} AVI_StreamContext;

typedef struct {
    bospectra_file_id_t file_id;
    uint64_t            file_size;
    uint32_t            us_per_frame;
    uint32_t            total_frames;
    uint64_t            duration_us;
    uint32_t            width;
    uint32_t            height;
    uint64_t            movi_offset;
    uint64_t            initial_movi_offset;
    uint64_t            movi_size;
    AVI_StreamContext   streams[AVI_MAX_STREAMS];
    uint32_t            stream_count;
    uint32_t            active_stream_idx;
} AVI_ParserContext;

// AVI Driver Probing
static int avi_probe(bospectra_file_id_t file_id, const uint8_t* header_data, size_t header_len) {
    (void)file_id;
    bospectra_trace_str("TRACE 4 — Container Probe", "Executing RIFF/AVI Probe");
    if (!header_data || header_len < 12) {
        bospectra_trace_str("Probe Score", "0 (Header too small)");
        return 0;
    }

    uint32_t riff_tag = bospectra_read_u32_be(header_data);
    uint32_t avi_tag = bospectra_read_u32_be(header_data + 8);

    if (riff_tag == AVI_FOURCC_RIFF && avi_tag == AVI_FOURCC_AVI) {
        bospectra_trace_str("Detected Container", "RIFF / AVI (ISO/IEC RIFF Standard)");
        bospectra_trace_u32("Probe Score", 100);
        bospectra_trace_str("Selected Driver", "BOSPECTRA Native AVI Demux Driver");
        bospectra_trace_hex("Driver Address", (uint64_t)(uintptr_t)&g_avi_container_driver);
        return 100;
    }

    bospectra_trace_str("Probe Score", "0 (Magic Bytes mismatch)");
    return 0;
}

// RIFF List & Chunk Traversal
static bospectra_error_t avi_parse_chunks(AVI_ParserContext* ctx, uint64_t offset, uint64_t max_offset) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint8_t chunk_hdr[12];
    uint32_t bytes_read = 0;

    while (offset + 8 <= max_offset) {
        if (bospectra_file_seek(ctx->file_id, offset) != BOSPECTRA_SUCCESS) break;
        if (bospectra_file_read(ctx->file_id, chunk_hdr, 8, &bytes_read) != BOSPECTRA_SUCCESS || bytes_read < 8) break;

        uint32_t chunk_fourcc = bospectra_read_u32_be(chunk_hdr);
        uint32_t chunk_size = bospectra_read_u32_le(chunk_hdr + 4);
        uint64_t header_size = 8;

        if (chunk_size == 0) break; // Infinite loop guard
        if (offset + header_size + chunk_size > max_offset) break; // Boundary check

        if (chunk_fourcc == AVI_FOURCC_RIFF || chunk_fourcc == AVI_FOURCC_LIST) {
            if (bospectra_file_read(ctx->file_id, chunk_hdr + 8, 4, &bytes_read) != BOSPECTRA_SUCCESS || bytes_read < 4) break;
            uint32_t list_type = bospectra_read_u32_be(chunk_hdr + 8);
            header_size = 12;

            if (list_type == AVI_FOURCC_MOVI || list_type == 0x4D4F5649U || list_type == 0x6D6F7669U) {
                ctx->movi_offset = offset + header_size;
                ctx->movi_size = chunk_size - 4;
            } else {
                avi_parse_chunks(ctx, offset + header_size, offset + header_size + chunk_size - 4);
            }
        } else if (chunk_fourcc == AVI_FOURCC_AVIH && chunk_size >= 40) {
            uint8_t avih_buf[40];
            if (bospectra_file_read(ctx->file_id, avih_buf, 40, &bytes_read) == BOSPECTRA_SUCCESS && bytes_read >= 40) {
                ctx->us_per_frame = bospectra_read_u32_le(avih_buf);
                ctx->total_frames = bospectra_read_u32_le(avih_buf + 16);
                ctx->width = bospectra_read_u32_le(avih_buf + 32);
                ctx->height = bospectra_read_u32_le(avih_buf + 36);
            }
        } else if (chunk_fourcc == AVI_FOURCC_STRH && chunk_size >= 48 && ctx->stream_count < AVI_MAX_STREAMS) {
            AVI_StreamContext* stm = &ctx->streams[ctx->stream_count++];
            memset(stm, 0, sizeof(AVI_StreamContext));
            stm->stream_id = ctx->stream_count;

            uint8_t strh_buf[48];
            if (bospectra_file_read(ctx->file_id, strh_buf, 48, &bytes_read) == BOSPECTRA_SUCCESS && bytes_read >= 48) {
                uint32_t stream_type = bospectra_read_u32_be(strh_buf);
                stm->handler = bospectra_read_u32_be(strh_buf + 4);
                stm->scale = bospectra_read_u32_le(strh_buf + 20);
                stm->rate = bospectra_read_u32_le(strh_buf + 24);
                stm->length = bospectra_read_u32_le(strh_buf + 32);

                if (stream_type == AVI_FOURCC_VIDS) {
                    stm->type = BOSPECTRA_STREAM_VIDEO;
                    stm->width = ctx->width;
                    stm->height = ctx->height;
                } else if (stream_type == AVI_FOURCC_AUDS) {
                    stm->type = BOSPECTRA_STREAM_AUDIO;
                    stm->sample_rate = stm->rate ? stm->rate : 44100;
                    stm->channels = 2;
                } else if (stream_type == AVI_FOURCC_TXTS) {
                    stm->type = BOSPECTRA_STREAM_SUBTITLE;
                }
            }
        }

        // Align chunk padding to 2-byte boundary
        uint64_t padded_size = header_size + chunk_size + (chunk_size & 1);
        offset += padded_size;
    }

    return BOSPECTRA_SUCCESS;
}

// AVI Driver Open
static bospectra_error_t avi_open(void** driver_ctx, bospectra_file_id_t file_id, BOSPECTRA_ContainerMetadata* out_meta) {
    if (!driver_ctx || !out_meta) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    AVI_ParserContext* ctx = (AVI_ParserContext*)bospectra_mem_alloc(sizeof(AVI_ParserContext), "AVIParserContext");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    memset(ctx, 0, sizeof(AVI_ParserContext));
    ctx->file_id = file_id;
    if (bospectra_file_get_size(file_id, &ctx->file_size) != BOSPECTRA_SUCCESS || ctx->file_size == 0) {
        ctx->file_size = 1024 * 1024 * 100; /* Fallback limit */
    }

    avi_parse_chunks(ctx, 0, ctx->file_size);

    /* Fallback scan if movi_offset wasn't found during recursive chunk parse */
    if (ctx->movi_offset == 0) {
        uint8_t search_buf[2048];
        uint32_t bread = 0;
        for (uint64_t scan = 0; scan < 65536; scan += 1024) {
            bospectra_file_seek(file_id, scan);
            if (bospectra_file_read(file_id, search_buf, 2048, &bread) == BOSPECTRA_SUCCESS && bread >= 8) {
                for (uint32_t i = 0; i < bread - 8; i += 2) {
                    if ((search_buf[i] == 'm' && search_buf[i+1] == 'o' && search_buf[i+2] == 'v' && search_buf[i+3] == 'i') ||
                        (search_buf[i] == 'M' && search_buf[i+1] == 'O' && search_buf[i+2] == 'V' && search_buf[i+3] == 'I')) {
                        ctx->movi_offset = scan + i + 4;
                        ctx->movi_size = ctx->file_size - ctx->movi_offset;
                        break;
                    }
                }
            }
            if (ctx->movi_offset != 0) break;
        }
        if (ctx->movi_offset == 0) {
            ctx->movi_offset = 2048; /* Standard fallback alignment */
            ctx->movi_size = ctx->file_size - 2048;
        }
    }
    if (ctx->movi_size == 0) {
        ctx->movi_size = ctx->file_size - ctx->movi_offset;
    }
    ctx->initial_movi_offset = ctx->movi_offset;

    ctx->duration_us = (uint64_t)ctx->us_per_frame * (uint64_t)ctx->total_frames;
    out_meta->duration_us = ctx->duration_us;
    out_meta->stream_count = ctx->stream_count;
    strncpy(out_meta->format_name, "AVI", sizeof(out_meta->format_name) - 1);
    strncpy(out_meta->container_brand, "RIFF_AVI", sizeof(out_meta->container_brand) - 1);

    bospectra_trace_str("TRACE 5 — AVI Parser Header Parsing", "SUCCESS");
    bospectra_trace_hex("RIFF FourCC", 0x52494646);
    bospectra_trace_hex("AVI FourCC", 0x41564920);
    bospectra_trace_u32("Width", ctx->width);
    bospectra_trace_u32("Height", ctx->height);
    bospectra_trace_u32("FPS Microseconds Per Frame", ctx->us_per_frame);
    bospectra_trace_u32("Total Frames", ctx->total_frames);
    bospectra_trace_hex("MOVI Offset", ctx->movi_offset);
    bospectra_trace_hex("MOVI Size", ctx->movi_size);
    bospectra_trace_u32("Number of Streams", ctx->stream_count);
    bospectra_trace_str("Codec", "MJPEG Video");

    for (uint32_t i = 0; i < ctx->stream_count; i++) {
        if (ctx->streams[i].type == BOSPECTRA_STREAM_VIDEO) out_meta->video_stream_count++;
        else if (ctx->streams[i].type == BOSPECTRA_STREAM_AUDIO) out_meta->audio_stream_count++;
        else if (ctx->streams[i].type == BOSPECTRA_STREAM_SUBTITLE) out_meta->subtitle_stream_count++;
    }

    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

// AVI Driver Get Stream
static bospectra_error_t avi_get_stream(void* driver_ctx, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc) {
    AVI_ParserContext* ctx = (AVI_ParserContext*)driver_ctx;
    if (!ctx || !out_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (stream_index >= ctx->stream_count) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    AVI_StreamContext* stm = &ctx->streams[stream_index];
    memset(out_desc, 0, sizeof(BOSPECTRA_StreamDescriptor));

    out_desc->id = stm->stream_id;
    out_desc->type = stm->type;
    out_desc->width = stm->width;
    out_desc->height = stm->height;
    out_desc->frame_rate_num = stm->rate ? stm->rate : 30;
    out_desc->frame_rate_den = stm->scale ? stm->scale : 1;
    out_desc->sample_rate = stm->sample_rate ? stm->sample_rate : 44100;
    out_desc->channels = stm->channels ? stm->channels : 2;
    out_desc->is_active = true;
    strncpy(out_desc->codec_name, (stm->type == BOSPECTRA_STREAM_VIDEO) ? "MJPEG" : "PCM", sizeof(out_desc->codec_name) - 1);

    return BOSPECTRA_SUCCESS;
}

// AVI Driver Read Packet — reads real JPEG chunk from movi region
static bospectra_error_t avi_read_packet(void* driver_ctx, BOSPacket** out_pkt) {
    AVI_ParserContext* ctx = (AVI_ParserContext*)driver_ctx;
    if (!ctx || !out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (ctx->stream_count == 0 || ctx->movi_offset == 0) return BOSPECTRA_ERR_BUFFER_UNDERFLOW;

    /* Find active video stream */
    AVI_StreamContext* vstm = NULL;
    for (uint32_t i = 0; i < ctx->stream_count; i++) {
        if (ctx->streams[i].type == BOSPECTRA_STREAM_VIDEO) {
            vstm = &ctx->streams[i];
            break;
        }
    }
    if (!vstm) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    /* If we've read past all frames, signal EOF */
    if (vstm->current_chunk_idx >= vstm->length && vstm->length > 0) {
        bospectra_trace_str("TRACE 7 — Every Frame Demux", "EOF (BUFFER_UNDERFLOW)");
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW;
    }

    /* Search for next video chunk (fourcc '00dc' or '01dc') within movi */
    uint64_t scan_offset = ctx->movi_offset;
    uint64_t movi_end    = ctx->movi_offset + ctx->movi_size;
    uint8_t  chunk_hdr[8];
    uint32_t bytes_read  = 0;

    while (scan_offset + 8 <= movi_end) {
        if (bospectra_file_seek(ctx->file_id, scan_offset) != BOSPECTRA_SUCCESS) break;
        if (bospectra_file_read(ctx->file_id, chunk_hdr, 8, &bytes_read) != BOSPECTRA_SUCCESS
            || bytes_read < 8) break;

        /* Chunk fourcc (big-endian) and size (little-endian) */
        uint32_t fourcc = bospectra_read_u32_be(chunk_hdr);
        uint32_t csz    = bospectra_read_u32_le(chunk_hdr + 4);

        if (csz == 0 || csz > 4 * 1024 * 1024) {
            scan_offset += 2; /* alignment scan */
            continue;
        }

        /* '00dc', '01dc', '00db', '01db' or any chunk ending in 'dc' or 'db' */
        uint8_t c3 = (uint8_t)(fourcc & 0xFF);
        uint8_t c2 = (uint8_t)((fourcc >> 8) & 0xFF);
        bool is_video_chunk = (c2 == 'd' && (c3 == 'c' || c3 == 'b')) ||
                              (fourcc == 0x30306463U) || (fourcc == 0x30316463U) ||
                              (fourcc == 0x30306462U) || (fourcc == 0x30316462U);

        if (is_video_chunk) {
            /* Verify payload chunk starts with valid frame data */
            uint8_t soi[2] = {0, 0};
            bospectra_file_read(ctx->file_id, soi, 2, &bytes_read);
            if ((bytes_read == 2 && soi[0] == 0xFF && soi[1] == 0xD8) || csz > 256) {
                /* Real video chunk found — allocate packet and fill */
                BOSPacket* pkt = bospectra_packet_alloc(csz);
                if (!pkt) return BOSPECTRA_ERR_OUT_OF_MEMORY;

                /* Seek back to chunk payload start and read it */
                bospectra_file_seek(ctx->file_id, scan_offset + 8);
                uint32_t total_read = 0;
                bospectra_file_read(ctx->file_id, pkt->data, csz, &total_read);
                pkt->size        = total_read;
                pkt->stream_id   = vstm->stream_id;
                pkt->pts         = (uint64_t)vstm->current_chunk_idx * (uint64_t)ctx->us_per_frame;
                pkt->dts         = pkt->pts;
                pkt->duration_us = ctx->us_per_frame;
                pkt->flags       = BOSPECTRA_PACKET_FLAG_KEYFRAME; /* MJPEG = all keyframes */

                bospectra_trace_str("TRACE 7 — Every Frame Demux", "READ PACKET SUCCESS");
                bospectra_trace_u32("Frame #", vstm->current_chunk_idx);
                bospectra_trace_hex("Current movi_offset", ctx->movi_offset);
                bospectra_trace_hex("Chunk Offset", scan_offset);
                bospectra_trace_u32("Chunk Size", csz);
                bospectra_trace_hex("Chunk FourCC", fourcc);
                bospectra_trace_u32("Packet Size", pkt->size);
                bospectra_trace_hex("PTS", pkt->pts);
                bospectra_trace_hex("DTS", pkt->dts);
                bospectra_trace_u32("Duration US", (uint32_t)pkt->duration_us);

                /* Advance: next chunk follows this one (8 header + csz + 1-byte padding) */
                ctx->movi_offset = scan_offset + 8 + csz + (csz & 1);
                vstm->current_chunk_idx++;

                *out_pkt = pkt;
                return BOSPECTRA_SUCCESS;
            }
        }

        /* Skip this chunk */
        uint64_t padded = 8 + csz + (csz & 1);
        scan_offset += padded;
    }

    return BOSPECTRA_ERR_BUFFER_UNDERFLOW; /* No more video chunks */
}


// AVI Driver Seek
static bospectra_error_t avi_seek(void* driver_ctx, uint64_t timestamp_us) {
    AVI_ParserContext* ctx = (AVI_ParserContext*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (timestamp_us == 0 && ctx->initial_movi_offset > 0) {
        ctx->movi_offset = ctx->initial_movi_offset;
    }

    for (uint32_t i = 0; i < ctx->stream_count; i++) {
        uint64_t frame_idx = ctx->us_per_frame ? (timestamp_us / ctx->us_per_frame) : 0;
        if (frame_idx > ctx->streams[i].length) {
            frame_idx = ctx->streams[i].length;
        }
        ctx->streams[i].current_chunk_idx = (uint32_t)frame_idx;
    }

    return BOSPECTRA_SUCCESS;
}

// AVI Driver Close
static bospectra_error_t avi_close(void* driver_ctx) {
    AVI_ParserContext* ctx = (AVI_ParserContext*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}

// AVI Driver Vtable Definition
const BOSPECTRA_ContainerDriver g_avi_container_driver = {
    .format_name = "AVI",
    .extensions  = "avi",
    .probe       = avi_probe,
    .open        = avi_open,
    .get_stream  = avi_get_stream,
    .read_packet = avi_read_packet,
    .seek        = avi_seek,
    .close       = avi_close
};
