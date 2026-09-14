/*
 * ============================================================================
 * ATOMS OS — Userspace Native MP4 / ISO Base Media Demuxer
 * userspace/libbos_media/demux/mp4_demuxer.h
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Derived from minimp4 (CC0 1.0 Universal / Public Domain)
 *
 * Fully dynamic ISO box parser operating over BOSMediaStream.
 * Zero hardcoded file offsets; dynamically parses ftyp, moov, trak, avcC,
 * stsz, stco, co64, stsc, stts, and stss.
 * ============================================================================
 */

#ifndef MP4_DEMUXER_H
#define MP4_DEMUXER_H

#include "../include/bos_media_stream.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MP4_MAX_TRACKS   8
#define MP4_MAX_SPS_LEN  256
#define MP4_MAX_PPS_LEN  128

typedef enum {
    MP4_TRACK_UNKNOWN = 0,
    MP4_TRACK_VIDEO   = 1,
    MP4_TRACK_AUDIO   = 2
} MP4TrackType;

typedef struct {
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t desc_index;
} MP4StscEntry;

typedef struct {
    uint32_t sample_count;
    uint32_t sample_delta;
} MP4SttsEntry;

typedef struct {
    uint32_t      track_id;
    MP4TrackType  type;
    uint32_t      codec_fourcc;
    
    // Video attributes
    uint32_t      width;
    uint32_t      height;
    uint32_t      nal_length_size;
    uint8_t       sps[MP4_MAX_SPS_LEN];
    uint32_t      sps_len;
    uint8_t       sps_profile;
    uint8_t       sps_level;
    uint8_t       entropy_coding_mode; // 0=CAVLC, 1=CABAC
    uint8_t       pps[MP4_MAX_PPS_LEN];
    uint32_t      pps_len;

    // Audio attributes
    uint32_t      sample_rate;
    uint8_t       channels;
    uint8_t       bits_per_sample;

    // Timing
    uint32_t      timescale;
    uint64_t      duration_us;

    // Sample index tables
    uint32_t      sample_count;
    uint32_t*     sample_sizes;
    uint32_t      chunk_count;
    uint64_t*     chunk_offsets;
    uint32_t      stsc_count;
    MP4StscEntry* stsc_table;
    uint32_t      stts_count;
    MP4SttsEntry* stts_table;
    uint32_t      stss_count;
    uint32_t*     stss_table;

    // Sequential sample lookup cache (O(1) sequential access)
    uint32_t      cached_sample_idx;
    uint32_t      cached_chunk_idx;
    uint32_t      cached_chunk_sample_start;
    uint32_t      cached_stsc_idx;
    uint64_t      cached_sample_offset;
    bool          has_sample_cache;
} MP4Track;

typedef struct {
    BOSMediaStream* stream;
    uint64_t        file_size;
    uint32_t        timescale;
    uint64_t        duration_us;
    
    uint32_t        track_count;
    MP4Track        tracks[MP4_MAX_TRACKS];
    int32_t         video_track_idx;
    int32_t         audio_track_idx;
} MP4Demuxer;

int  mp4_demuxer_probe(BOSMediaStream* stream);
int  mp4_demuxer_open(BOSMediaStream* stream, MP4Demuxer* demuxer);
void mp4_demuxer_close(MP4Demuxer* demuxer);

int  mp4_demuxer_get_sample_info(MP4Demuxer* demuxer, uint32_t track_idx, uint32_t sample_idx,
                                 uint64_t* out_offset, uint32_t* out_size, uint64_t* out_pts_us, bool* out_is_keyframe);

int  mp4_demuxer_read_sample(MP4Demuxer* demuxer, uint32_t track_idx, uint32_t sample_idx,
                             uint8_t* out_buffer, uint32_t buffer_size, uint32_t* out_bytes_read);

#ifdef __cplusplus
}
#endif

#endif /* MP4_DEMUXER_H */
