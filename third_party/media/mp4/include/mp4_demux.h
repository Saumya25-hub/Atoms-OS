#ifndef MP4_DEMUX_H
#define MP4_DEMUX_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/media/bospectra/include/bospectra_types.h"
#include "kernel/media/bospectra/include/bospectra_errors.h"
#include "kernel/media/bospectra/file/bospectra_file.h"

#define MP4_MAX_TRACKS 4
#define MP4_MAX_SPS_LEN 256
#define MP4_MAX_PPS_LEN 256

typedef struct {
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t desc_index;
} MP4_StscEntry;

typedef struct {
    uint32_t sample_count;
    uint32_t sample_delta;
} MP4_SttsEntry;

typedef struct {
    uint32_t track_id;
    bospectra_stream_type_t type; /* VIDEO or AUDIO */
    uint32_t codec_fourcc;        /* e.g. 'avc1', 'mp4a' */
    uint32_t width;
    uint32_t height;
    uint32_t sample_rate;
    uint8_t  channels;
    uint32_t timescale;
    uint64_t duration_us;

    /* Sample tables */
    uint32_t sample_count;
    uint32_t chunk_count;
    uint32_t stsc_count;
    uint32_t stts_count;
    uint32_t stss_count;

    uint32_t* sample_sizes;      /* from stsz */
    uint64_t* chunk_offsets;     /* from stco / co64 */
    MP4_StscEntry* stsc_table;   /* from stsc */
    MP4_SttsEntry* stts_table;   /* from stts */
    uint32_t* stss_table;        /* from stss (sync/keyframe indices, 1-based) */

    /* H.264 Extradata (from avcC box) */
    uint8_t  sps[MP4_MAX_SPS_LEN];
    uint16_t sps_len;
    uint8_t  pps[MP4_MAX_PPS_LEN];
    uint16_t pps_len;
    uint8_t  nal_length_size;    /* typically 4 */

    /* Playback State */
    uint32_t current_sample_idx;
} MP4_DemuxTrack;

typedef struct {
    bospectra_file_id_t file_id;
    uint64_t file_size;
    uint32_t timescale;
    uint64_t duration_us;
    uint32_t track_count;
    MP4_DemuxTrack tracks[MP4_MAX_TRACKS];
    int32_t video_track_idx;
    int32_t audio_track_idx;
} MP4_DemuxContext;

/* Core API */
bospectra_error_t mp4_demux_probe(bospectra_file_id_t file_id);
bospectra_error_t mp4_demux_open(bospectra_file_id_t file_id, MP4_DemuxContext** out_ctx);
bospectra_error_t mp4_demux_get_sample_info(MP4_DemuxContext* ctx, uint32_t track_idx, uint32_t sample_idx,
                                           uint64_t* out_file_offset, uint32_t* out_size,
                                           uint64_t* out_pts_us, bool* out_is_keyframe);
bospectra_error_t mp4_demux_read_sample(MP4_DemuxContext* ctx, uint32_t track_idx, uint32_t sample_idx,
                                        void* buffer, uint32_t buffer_size, uint32_t* out_bytes_read);
bospectra_error_t mp4_demux_close(MP4_DemuxContext* ctx);

#endif /* MP4_DEMUX_H */
