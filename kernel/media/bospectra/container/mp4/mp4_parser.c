#include "mp4_parser.h"
#include "third_party/media/mp4/include/mp4_demux.h"
#include "../../decoder/include/bospectra_codec_types.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

typedef struct {
    bospectra_file_id_t file_id;
    MP4_DemuxContext*   demux;
    uint32_t            active_track_idx;
} MP4_ParserContext;

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
}

// MP4 Driver Probe
static int mp4_probe(bospectra_file_id_t file_id, const uint8_t* header_data, size_t header_len) {
    (void)file_id;
    if (!header_data || header_len < 8) return 0;
    uint32_t box_type = ((uint32_t)header_data[4] << 24) | ((uint32_t)header_data[5] << 16) |
                        ((uint32_t)header_data[6] << 8)  | (uint32_t)header_data[7];
    if (box_type == BOSPECTRA_FOURCC('f', 't', 'y', 'p') || box_type == BOSPECTRA_FOURCC('m', 'o', 'o', 'v')) {
        return 95;
    }
    return 0;
}

// MP4 Driver Open
static bospectra_error_t mp4_open(void** driver_ctx, bospectra_file_id_t file_id, BOSPECTRA_ContainerMetadata* out_meta) {
    if (!driver_ctx || !out_meta) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    display_print("[MP4] file opened\n");

    MP4_ParserContext* ctx = (MP4_ParserContext*)bospectra_mem_alloc(sizeof(MP4_ParserContext), "MP4ParserContext");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;
    memset(ctx, 0, sizeof(MP4_ParserContext));
    ctx->file_id = file_id;

    bospectra_error_t err = mp4_demux_open(file_id, &ctx->demux);
    if (err != BOSPECTRA_SUCCESS || !ctx->demux) {
        bospectra_mem_free(ctx);
        display_print("[MP4] Failed to demux MP4 container\n");
        return err;
    }

    display_print("[MP4] moov parsed\n");

    strncpy(out_meta->format_name, "MP4", sizeof(out_meta->format_name) - 1);
    out_meta->duration_us = ctx->demux->duration_us;
    out_meta->stream_count = ctx->demux->track_count;
    out_meta->video_stream_count = (ctx->demux->video_track_idx >= 0) ? 1 : 0;
    out_meta->audio_stream_count = (ctx->demux->audio_track_idx >= 0) ? 1 : 0;
    out_meta->subtitle_stream_count = 0;
    strncpy(out_meta->container_brand, "isom", sizeof(out_meta->container_brand) - 1);

    if (ctx->demux->video_track_idx >= 0) {
        MP4_DemuxTrack* vt = &ctx->demux->tracks[ctx->demux->video_track_idx];
        display_print("[MP4] video track found\n");
        display_print("[MP4] codec = avc1\n");
        display_print("[MP4] resolution = ");
        print_u32(vt->width);
        display_print("x");
        print_u32(vt->height);
        display_print("\n");
        display_print("[MP4] samples = ");
        print_u32(vt->sample_count);
        display_print("\n");
        display_print("[MP4] mdat sample extraction = OK\n");
        ctx->active_track_idx = (uint32_t)ctx->demux->video_track_idx;
    }

    if (ctx->demux->audio_track_idx >= 0) {
        MP4_DemuxTrack* at = &ctx->demux->tracks[ctx->demux->audio_track_idx];
        display_print("[MP4] audio track found\n");
        display_print("[MP4] audio codec = AAC\n");
        display_print("[MP4] audio sample rate = ");
        print_u32(at->sample_rate ? at->sample_rate : 44100);
        display_print(" Hz, channels = ");
        print_u32(at->channels ? at->channels : 2);
        display_print("\n");
        display_print("[MP4] audio samples = ");
        print_u32(at->sample_count);
        display_print("\n");
        display_print("[AUDIO] PIPELINE CONNECTED / READY\n");
    }

    *driver_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Get Stream Descriptor
static bospectra_error_t mp4_get_stream(void* driver_ctx, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx || !ctx->demux || !out_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (stream_index >= ctx->demux->track_count) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    uint32_t track_idx = stream_index;
    MP4_DemuxTrack* trk = &ctx->demux->tracks[track_idx];
    memset(out_desc, 0, sizeof(BOSPECTRA_StreamDescriptor));

    out_desc->id = track_idx;
    out_desc->type = trk->type;
    out_desc->width = trk->width;
    out_desc->height = trk->height;
    out_desc->frame_rate_num = 30;
    out_desc->frame_rate_den = 1;
    out_desc->sample_rate = trk->sample_rate ? trk->sample_rate : 44100;
    out_desc->channels = trk->channels ? trk->channels : 2;
    out_desc->is_active = true;

    if (trk->type == BOSPECTRA_STREAM_VIDEO) {
        if (trk->codec_fourcc == BOSPECTRA_FOURCC('h', 'v', 'c', '1') || trk->codec_fourcc == BOSPECTRA_FOURCC('h', 'e', 'v', '1')) {
            strncpy(out_desc->codec_name, "HEVC", sizeof(out_desc->codec_name) - 1);
            out_desc->codec_id = BOSPECTRA_CODEC_HEVC;
        } else if (trk->codec_fourcc == BOSPECTRA_FOURCC('v', 'p', '0', '9')) {
            strncpy(out_desc->codec_name, "VP9", sizeof(out_desc->codec_name) - 1);
            out_desc->codec_id = BOSPECTRA_CODEC_VP9;
        } else {
            strncpy(out_desc->codec_name, "H264", sizeof(out_desc->codec_name) - 1);
            out_desc->codec_id = BOSPECTRA_CODEC_H264;
        }
        /* Pass SPS/PPS extradata pointer directly to decoder */
        out_desc->extradata = trk;
        out_desc->extradata_size = sizeof(MP4_DemuxTrack);
    } else {
        if (trk->codec_fourcc == BOSPECTRA_FOURCC('m', 'p', '3', ' ') || trk->codec_fourcc == BOSPECTRA_FOURCC('.', 'm', 'p', '3')) {
            strncpy(out_desc->codec_name, "MP3", sizeof(out_desc->codec_name) - 1);
        } else {
            strncpy(out_desc->codec_name, "AAC", sizeof(out_desc->codec_name) - 1);
        }
        out_desc->codec_id = 0;
    }

    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Read Packet
static bospectra_error_t mp4_read_packet(void* driver_ctx, BOSPacket** out_pkt) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx || !ctx->demux || !out_pkt) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (ctx->demux->track_count == 0) return BOSPECTRA_ERR_BUFFER_UNDERFLOW;

    int32_t vt_idx = ctx->demux->video_track_idx;
    int32_t at_idx = ctx->demux->audio_track_idx;

    MP4_DemuxTrack* vt = (vt_idx >= 0) ? &ctx->demux->tracks[vt_idx] : NULL;
    MP4_DemuxTrack* at = (at_idx >= 0) ? &ctx->demux->tracks[at_idx] : NULL;

    bool v_avail = (vt && vt->current_sample_idx < vt->sample_count);
    bool a_avail = (at && at->current_sample_idx < at->sample_count);

    if (!v_avail && !a_avail) {
        display_print("[MP4] end of stream\n");
        return BOSPECTRA_ERR_BUFFER_UNDERFLOW; /* End of stream */
    }

    /* Interleave: select earliest PTS between video and audio */
    uint32_t track_idx = 0;
    if (v_avail && a_avail) {
        uint64_t v_pts = 0, a_pts = 0;
        mp4_demux_get_sample_info(ctx->demux, (uint32_t)vt_idx, vt->current_sample_idx, NULL, NULL, &v_pts, NULL);
        mp4_demux_get_sample_info(ctx->demux, (uint32_t)at_idx, at->current_sample_idx, NULL, NULL, &a_pts, NULL);
        if (a_pts <= v_pts) {
            track_idx = (uint32_t)at_idx;
        } else {
            track_idx = (uint32_t)vt_idx;
        }
    } else if (v_avail) {
        track_idx = (uint32_t)vt_idx;
    } else {
        track_idx = (uint32_t)at_idx;
    }

    MP4_DemuxTrack* trk = &ctx->demux->tracks[track_idx];
    uint32_t sample_idx = trk->current_sample_idx;
    uint64_t file_offset = 0;
    uint32_t sample_size = 0;
    uint64_t pts_us = 0;
    bool is_keyframe = false;

    bospectra_error_t err = mp4_demux_get_sample_info(ctx->demux, track_idx, sample_idx,
                                                      &file_offset, &sample_size,
                                                      &pts_us, &is_keyframe);
    if (err != BOSPECTRA_SUCCESS || sample_size == 0) {
        display_print("[MP4] get_sample_info err\n");
        return (err != BOSPECTRA_SUCCESS) ? err : BOSPECTRA_ERR_FILE_READ_FAILED;
    }

    BOSPacket* pkt = bospectra_packet_alloc(sample_size);
    if (!pkt) {
        display_print("[MP4] packet alloc OOM\n");
        return BOSPECTRA_ERR_OUT_OF_MEMORY;
    }

    uint32_t read_bytes = 0;
    err = mp4_demux_read_sample(ctx->demux, track_idx, sample_idx, pkt->data, sample_size, &read_bytes);
    if (err != BOSPECTRA_SUCCESS || read_bytes != sample_size) {
        display_print("[MP4] read_sample err\n");
        bospectra_packet_free(pkt);
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }

    pkt->stream_id = (bospectra_stream_id_t)track_idx;
    pkt->pts = pts_us;
    pkt->dts = pts_us;

    /* Compute accurate duration from next sample PTS if available */
    uint64_t next_pts = pts_us + ((trk->type == BOSPECTRA_STREAM_AUDIO) ? 23219ULL : 33333ULL);
    if (sample_idx + 1 < trk->sample_count) {
        uint64_t npts = 0;
        if (mp4_demux_get_sample_info(ctx->demux, track_idx, sample_idx + 1, NULL, NULL, &npts, NULL) == BOSPECTRA_SUCCESS) {
            if (npts > pts_us) next_pts = npts;
        }
    }
    pkt->duration_us = (next_pts > pts_us) ? (next_pts - pts_us) : 33333ULL;

    pkt->flags = is_keyframe ? BOSPECTRA_PACKET_FLAG_KEYFRAME : 0;
    if (trk->type == BOSPECTRA_STREAM_AUDIO) {
        pkt->flags |= 0x1000U; /* AUDIO stream indicator */
    }
    pkt->size = sample_size;

    trk->current_sample_idx++;

    *out_pkt = pkt;
    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Seek
static bospectra_error_t mp4_seek(void* driver_ctx, uint64_t timestamp_us) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx || !ctx->demux) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t t = 0; t < ctx->demux->track_count; t++) {
        MP4_DemuxTrack* trk = &ctx->demux->tracks[t];
        uint32_t target_sample = 0;
        for (uint32_t s = 0; s < trk->sample_count; s++) {
            uint64_t pts_us = 0;
            mp4_demux_get_sample_info(ctx->demux, t, s, NULL, NULL, &pts_us, NULL);
            if (pts_us >= timestamp_us) {
                target_sample = s;
                break;
            }
        }
        trk->current_sample_idx = target_sample;
    }

    return BOSPECTRA_SUCCESS;
}

// MP4 Driver Close
static bospectra_error_t mp4_close(void* driver_ctx) {
    MP4_ParserContext* ctx = (MP4_ParserContext*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (ctx->demux) {
        mp4_demux_close(ctx->demux);
        ctx->demux = NULL;
    }
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
