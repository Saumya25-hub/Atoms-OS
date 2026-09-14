#include "../include/mp4_demux.h"
#include "kernel/media/bospectra/memory/bospectra_memory.h"
#include "kernel/media/bospectra/debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define FOURCC(a, b, c, d) \
    (((uint32_t)(uint8_t)(a) << 24) | ((uint32_t)(uint8_t)(b) << 16) | \
     ((uint32_t)(uint8_t)(c) << 8)  | ((uint32_t)(uint8_t)(d)))

static inline uint16_t read_u16_be(const uint8_t* p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

static inline uint32_t read_u32_be(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static inline uint64_t read_u64_be(const uint8_t* p) {
    return ((uint64_t)read_u32_be(p) << 32) | (uint64_t)read_u32_be(p + 4);
}

bospectra_error_t mp4_demux_probe(bospectra_file_id_t file_id) {
    uint8_t buf[16];
    uint32_t read_bytes = 0;
    if (bospectra_file_seek(file_id, 0) != BOSPECTRA_SUCCESS) return BOSPECTRA_ERR_FILE_READ_FAILED;
    if (bospectra_file_read(file_id, buf, 16, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) {
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }
    uint32_t box_type = read_u32_be(buf + 4);
    if (box_type == FOURCC('f', 't', 'y', 'p') || box_type == FOURCC('m', 'o', 'o', 'v')) {
        return BOSPECTRA_SUCCESS;
    }
    return BOSPECTRA_ERR_UNSUPPORTED_FORMAT;
}

static bospectra_error_t parse_avcc(MP4_DemuxTrack* trk, const uint8_t* data, uint32_t size) {
    if (size < 7) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    /* data[0] = version (1)
     * data[1] = profile
     * data[2] = profile_compat
     * data[3] = level
     * data[4] = 0xFC | (nal_length_size - 1)
     * data[5] = 0xE0 | (num_sps & 0x1F)
     */
    trk->nal_length_size = (data[4] & 0x03) + 1;
    uint32_t num_sps = data[5] & 0x1F;
    uint32_t offset = 6;

    if (num_sps > 0 && offset + 2 <= size) {
        uint16_t sps_len = read_u16_be(data + offset);
        offset += 2;
        if (offset + sps_len <= size && sps_len <= MP4_MAX_SPS_LEN) {
            memcpy(trk->sps, data + offset, sps_len);
            trk->sps_len = sps_len;
            offset += sps_len;
        }
    }

    if (offset < size) {
        uint32_t num_pps = data[offset++];
        if (num_pps > 0 && offset + 2 <= size) {
            uint16_t pps_len = read_u16_be(data + offset);
            offset += 2;
            if (offset + pps_len <= size && pps_len <= MP4_MAX_PPS_LEN) {
                memcpy(trk->pps, data + offset, pps_len);
                trk->pps_len = pps_len;
            }
        }
    }

    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t parse_stsd(MP4_DemuxContext* ctx, MP4_DemuxTrack* trk, uint64_t box_start, uint64_t box_end) {
    uint8_t hdr[8];
    uint32_t read_bytes = 0;
    if (bospectra_file_seek(ctx->file_id, box_start) != BOSPECTRA_SUCCESS) return BOSPECTRA_ERR_FILE_READ_FAILED;
    if (bospectra_file_read(ctx->file_id, hdr, 8, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) {
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }
    uint32_t entry_count = read_u32_be(hdr + 4);
    uint64_t cur = box_start + 8;

    for (uint32_t i = 0; i < entry_count && cur < box_end; i++) {
        uint8_t entry_hdr[8];
        if (bospectra_file_seek(ctx->file_id, cur) != BOSPECTRA_SUCCESS) break;
        if (bospectra_file_read(ctx->file_id, entry_hdr, 8, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) break;
        uint32_t entry_size = read_u32_be(entry_hdr);
        uint32_t format = read_u32_be(entry_hdr + 4);
        trk->codec_fourcc = format;

        if (format == FOURCC('a', 'v', 'c', '1')) {
            trk->type = BOSPECTRA_STREAM_VIDEO;
            /* VisualSampleEntry: 78 bytes total before sub-boxes */
            uint8_t visual_hdr[78];
            if (bospectra_file_seek(ctx->file_id, cur) == BOSPECTRA_SUCCESS &&
                bospectra_file_read(ctx->file_id, visual_hdr, 78, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 78) {
                trk->width  = read_u16_be(visual_hdr + 32);
                trk->height = read_u16_be(visual_hdr + 34);
            }

            /* Search sub-boxes for avcC: VisualSampleEntry is 8 bytes box header + 78 bytes = 86 bytes */
            uint64_t sub_offset = cur + 86;
            bool found_avcc = false;
            while (sub_offset + 8 <= cur + entry_size) {
                uint8_t sub_hdr[8];
                if (bospectra_file_seek(ctx->file_id, sub_offset) != BOSPECTRA_SUCCESS) break;
                if (bospectra_file_read(ctx->file_id, sub_hdr, 8, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) break;
                uint32_t sub_size = read_u32_be(sub_hdr);
                uint32_t sub_type = read_u32_be(sub_hdr + 4);
                if (sub_size < 8) break;

                if (sub_type == FOURCC('a', 'v', 'c', 'C')) {
                    uint32_t avcc_payload_size = sub_size - 8;
                    if (avcc_payload_size > 1024) avcc_payload_size = 1024;
                    uint8_t* avcc_buf = (uint8_t*)bospectra_mem_alloc(avcc_payload_size, "avcC_buf");
                    if (avcc_buf) {
                        if (bospectra_file_read(ctx->file_id, avcc_buf, avcc_payload_size, &read_bytes) == BOSPECTRA_SUCCESS) {
                            parse_avcc(trk, avcc_buf, read_bytes);
                        }
                        bospectra_mem_free(avcc_buf);
                    }
                    found_avcc = true;
                    break;
                }
                sub_offset += sub_size;
            }

            /* Fallback byte scanner for avcC in case of non-standard padding */
            if (!found_avcc && entry_size > 8) {
                uint32_t scan_len = (entry_size > 2048) ? 2048 : (uint32_t)entry_size;
                uint8_t* scan_buf = (uint8_t*)bospectra_mem_alloc(scan_len, "scan_buf");
                if (scan_buf) {
                    if (bospectra_file_seek(ctx->file_id, cur) == BOSPECTRA_SUCCESS &&
                        bospectra_file_read(ctx->file_id, scan_buf, scan_len, &read_bytes) == BOSPECTRA_SUCCESS) {
                        for (uint32_t bi = 8; bi + 8 <= read_bytes; bi++) {
                            if (scan_buf[bi+4] == 'a' && scan_buf[bi+5] == 'v' &&
                                scan_buf[bi+6] == 'c' && scan_buf[bi+7] == 'C') {
                                uint32_t sub_size = read_u32_be(scan_buf + bi);
                                uint32_t avcc_payload_size = (sub_size >= 8) ? (sub_size - 8) : 0;
                                if (avcc_payload_size > 1024) avcc_payload_size = 1024;
                                if (bi + 8 + avcc_payload_size <= read_bytes) {
                                    parse_avcc(trk, scan_buf + bi + 8, avcc_payload_size);
                                }
                                break;
                            }
                        }
                    }
                    bospectra_mem_free(scan_buf);
                }
            }
        } else if (format == FOURCC('h', 'v', 'c', '1') || format == FOURCC('h', 'e', 'v', '1')) {
            trk->type = BOSPECTRA_STREAM_VIDEO;
            uint8_t visual_hdr[78];
            if (bospectra_file_seek(ctx->file_id, cur) == BOSPECTRA_SUCCESS &&
                bospectra_file_read(ctx->file_id, visual_hdr, 78, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 78) {
                trk->width  = read_u16_be(visual_hdr + 32);
                trk->height = read_u16_be(visual_hdr + 34);
            }
        } else if (format == FOURCC('v', 'p', '0', '9') || format == FOURCC('v', 'p', '0', '8')) {
            trk->type = BOSPECTRA_STREAM_VIDEO;
            uint8_t visual_hdr[78];
            if (bospectra_file_seek(ctx->file_id, cur) == BOSPECTRA_SUCCESS &&
                bospectra_file_read(ctx->file_id, visual_hdr, 78, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 78) {
                trk->width  = read_u16_be(visual_hdr + 32);
                trk->height = read_u16_be(visual_hdr + 34);
            }
        } else if (format == FOURCC('m', 'p', '4', 'a') || format == FOURCC('m', 'p', '3', ' ') || format == FOURCC('.', 'm', 'p', '3')) {
            trk->type = BOSPECTRA_STREAM_AUDIO;
            uint8_t audio_hdr[28];
            if (bospectra_file_seek(ctx->file_id, cur) == BOSPECTRA_SUCCESS &&
                bospectra_file_read(ctx->file_id, audio_hdr, 28, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 28) {
                trk->channels = (uint8_t)read_u16_be(audio_hdr + 16);
                trk->sample_rate = read_u32_be(audio_hdr + 22) >> 16;
                if (trk->sample_rate == 0) trk->sample_rate = 44100;
                if (trk->channels == 0) trk->channels = 2;
            }
        }

        if (entry_size == 0) break;
        cur += entry_size;
    }
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t parse_boxes_recursive(MP4_DemuxContext* ctx, uint64_t start_offset, uint64_t end_offset, int depth) {
    if (depth > 8) return BOSPECTRA_SUCCESS; /* Max recursion guard */
    uint64_t offset = start_offset;
    uint32_t read_bytes = 0;

    while (offset + 8 <= end_offset) {
        uint8_t hdr[16];
        if (bospectra_file_seek(ctx->file_id, offset) != BOSPECTRA_SUCCESS) break;
        if (bospectra_file_read(ctx->file_id, hdr, 8, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) break;

        uint64_t box_size = read_u32_be(hdr);
        uint32_t box_type = read_u32_be(hdr + 4);
        uint32_t hdr_size = 8;

        if (box_size == 1) {
            if (bospectra_file_read(ctx->file_id, hdr + 8, 8, &read_bytes) != BOSPECTRA_SUCCESS || read_bytes < 8) break;
            box_size = read_u64_be(hdr + 8);
            hdr_size = 16;
        } else if (box_size == 0) {
            box_size = end_offset - offset;
        }

        if (box_size < hdr_size || offset + box_size > end_offset) break;

        MP4_DemuxTrack* cur_track = (ctx->track_count > 0) ? &ctx->tracks[ctx->track_count - 1] : NULL;

        if (box_type == FOURCC('m', 'o', 'o', 'v') ||
            box_type == FOURCC('m', 'd', 'i', 'a') ||
            box_type == FOURCC('m', 'i', 'n', 'f') ||
            box_type == FOURCC('s', 't', 'b', 'l')) {
            parse_boxes_recursive(ctx, offset + hdr_size, offset + box_size, depth + 1);
        } else if (box_type == FOURCC('t', 'r', 'a', 'k')) {
            if (ctx->track_count < MP4_MAX_TRACKS) {
                MP4_DemuxTrack* t = &ctx->tracks[ctx->track_count++];
                memset(t, 0, sizeof(MP4_DemuxTrack));
                t->track_id = ctx->track_count;
                t->type = BOSPECTRA_STREAM_VIDEO; /* Default */
                parse_boxes_recursive(ctx, offset + hdr_size, offset + box_size, depth + 1);
            }
        } else if (box_type == FOURCC('m', 'v', 'h', 'd')) {
            uint8_t mvhd_buf[32];
            if (bospectra_file_read(ctx->file_id, mvhd_buf, 32, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 24) {
                uint8_t ver = mvhd_buf[0];
                if (ver == 0) {
                    ctx->timescale = read_u32_be(mvhd_buf + 12);
                    uint32_t dur = read_u32_be(mvhd_buf + 16);
                    ctx->duration_us = (ctx->timescale > 0) ? ((uint64_t)dur * 1000000ULL / ctx->timescale) : 0;
                } else if (ver == 1 && read_bytes >= 32) {
                    ctx->timescale = read_u32_be(mvhd_buf + 20);
                    uint64_t dur = read_u64_be(mvhd_buf + 24);
                    ctx->duration_us = (ctx->timescale > 0) ? (dur * 1000000ULL / ctx->timescale) : 0;
                }
            }
        } else if (box_type == FOURCC('m', 'd', 'h', 'd') && cur_track) {
            uint8_t mdhd_buf[32];
            if (bospectra_file_read(ctx->file_id, mdhd_buf, 32, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 24) {
                uint8_t ver = mdhd_buf[0];
                if (ver == 0) {
                    cur_track->timescale = read_u32_be(mdhd_buf + 12);
                    uint32_t dur = read_u32_be(mdhd_buf + 16);
                    cur_track->duration_us = (cur_track->timescale > 0) ? ((uint64_t)dur * 1000000ULL / cur_track->timescale) : 0;
                }
            }
        } else if (box_type == FOURCC('h', 'd', 'l', 'r') && cur_track) {
            uint8_t hdlr_buf[16];
            if (bospectra_file_read(ctx->file_id, hdlr_buf, 16, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 12) {
                uint32_t handler = read_u32_be(hdlr_buf + 8);
                if (handler == FOURCC('v', 'i', 'd', 'e')) {
                    cur_track->type = BOSPECTRA_STREAM_VIDEO;
                } else if (handler == FOURCC('s', 'o', 'u', 'n')) {
                    cur_track->type = BOSPECTRA_STREAM_AUDIO;
                }
            }
        } else if (box_type == FOURCC('s', 't', 's', 'd') && cur_track) {
            parse_stsd(ctx, cur_track, offset + hdr_size, offset + box_size);
        } else if (box_type == FOURCC('s', 't', 's', 'z') && cur_track) {
            uint8_t stsz_hdr[12];
            if (bospectra_file_read(ctx->file_id, stsz_hdr, 12, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 12) {
                uint32_t uniform_size = read_u32_be(stsz_hdr + 4);
                uint32_t count = read_u32_be(stsz_hdr + 8);
                cur_track->sample_count = count;
                if (count > 0 && count < 100000) {
                    cur_track->sample_sizes = (uint32_t*)bospectra_mem_alloc(count * sizeof(uint32_t), "stsz_table");
                    if (cur_track->sample_sizes) {
                        if (uniform_size > 0) {
                            for (uint32_t si = 0; si < count; si++) cur_track->sample_sizes[si] = uniform_size;
                        } else {
                            uint32_t table_bytes = count * 4;
                            uint8_t* raw_sizes = (uint8_t*)bospectra_mem_alloc(table_bytes, "raw_sizes");
                            if (raw_sizes) {
                                if (bospectra_file_read(ctx->file_id, raw_sizes, table_bytes, &read_bytes) == BOSPECTRA_SUCCESS) {
                                    for (uint32_t si = 0; si < count; si++) {
                                        cur_track->sample_sizes[si] = read_u32_be(raw_sizes + si * 4);
                                    }
                                }
                                bospectra_mem_free(raw_sizes);
                            }
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 'c', 'o') && cur_track) {
            uint8_t stco_hdr[8];
            if (bospectra_file_read(ctx->file_id, stco_hdr, 8, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 8) {
                uint32_t count = read_u32_be(stco_hdr + 4);
                cur_track->chunk_count = count;
                if (count > 0 && count < 100000) {
                    cur_track->chunk_offsets = (uint64_t*)bospectra_mem_alloc(count * sizeof(uint64_t), "stco_table");
                    if (cur_track->chunk_offsets) {
                        uint32_t table_bytes = count * 4;
                        uint8_t* raw_offsets = (uint8_t*)bospectra_mem_alloc(table_bytes, "raw_offsets");
                        if (raw_offsets) {
                            if (bospectra_file_read(ctx->file_id, raw_offsets, table_bytes, &read_bytes) == BOSPECTRA_SUCCESS) {
                                for (uint32_t ci = 0; ci < count; ci++) {
                                    cur_track->chunk_offsets[ci] = (uint64_t)read_u32_be(raw_offsets + ci * 4);
                                }
                            }
                            bospectra_mem_free(raw_offsets);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('c', 'o', '6', '4') && cur_track) {
            uint8_t co64_hdr[8];
            if (bospectra_file_read(ctx->file_id, co64_hdr, 8, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 8) {
                uint32_t count = read_u32_be(co64_hdr + 4);
                cur_track->chunk_count = count;
                if (count > 0 && count < 100000) {
                    cur_track->chunk_offsets = (uint64_t*)bospectra_mem_alloc(count * sizeof(uint64_t), "co64_table");
                    if (cur_track->chunk_offsets) {
                        uint32_t table_bytes = count * 8;
                        uint8_t* raw_offsets = (uint8_t*)bospectra_mem_alloc(table_bytes, "raw_offsets64");
                        if (raw_offsets) {
                            if (bospectra_file_read(ctx->file_id, raw_offsets, table_bytes, &read_bytes) == BOSPECTRA_SUCCESS) {
                                for (uint32_t ci = 0; ci < count; ci++) {
                                    cur_track->chunk_offsets[ci] = read_u64_be(raw_offsets + ci * 8);
                                }
                            }
                            bospectra_mem_free(raw_offsets);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 's', 'c') && cur_track) {
            uint8_t stsc_hdr[8];
            if (bospectra_file_read(ctx->file_id, stsc_hdr, 8, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 8) {
                uint32_t count = read_u32_be(stsc_hdr + 4);
                cur_track->stsc_count = count;
                if (count > 0 && count < 10000) {
                    cur_track->stsc_table = (MP4_StscEntry*)bospectra_mem_alloc(count * sizeof(MP4_StscEntry), "stsc_table");
                    if (cur_track->stsc_table) {
                        uint32_t table_bytes = count * 12;
                        uint8_t* raw_sc = (uint8_t*)bospectra_mem_alloc(table_bytes, "raw_sc");
                        if (raw_sc) {
                            if (bospectra_file_read(ctx->file_id, raw_sc, table_bytes, &read_bytes) == BOSPECTRA_SUCCESS) {
                                for (uint32_t si = 0; si < count; si++) {
                                    cur_track->stsc_table[si].first_chunk = read_u32_be(raw_sc + si * 12);
                                    cur_track->stsc_table[si].samples_per_chunk = read_u32_be(raw_sc + si * 12 + 4);
                                    cur_track->stsc_table[si].desc_index = read_u32_be(raw_sc + si * 12 + 8);
                                }
                            }
                            bospectra_mem_free(raw_sc);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 't', 's') && cur_track) {
            uint8_t stts_hdr[8];
            if (bospectra_file_read(ctx->file_id, stts_hdr, 8, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 8) {
                uint32_t count = read_u32_be(stts_hdr + 4);
                cur_track->stts_count = count;
                if (count > 0 && count < 10000) {
                    cur_track->stts_table = (MP4_SttsEntry*)bospectra_mem_alloc(count * sizeof(MP4_SttsEntry), "stts_table");
                    if (cur_track->stts_table) {
                        uint32_t table_bytes = count * 8;
                        uint8_t* raw_ts = (uint8_t*)bospectra_mem_alloc(table_bytes, "raw_ts");
                        if (raw_ts) {
                            if (bospectra_file_read(ctx->file_id, raw_ts, table_bytes, &read_bytes) == BOSPECTRA_SUCCESS) {
                                for (uint32_t si = 0; si < count; si++) {
                                    cur_track->stts_table[si].sample_count = read_u32_be(raw_ts + si * 8);
                                    cur_track->stts_table[si].sample_delta = read_u32_be(raw_ts + si * 8 + 4);
                                }
                            }
                            bospectra_mem_free(raw_ts);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 's', 's') && cur_track) {
            uint8_t stss_hdr[8];
            if (bospectra_file_read(ctx->file_id, stss_hdr, 8, &read_bytes) == BOSPECTRA_SUCCESS && read_bytes >= 8) {
                uint32_t count = read_u32_be(stss_hdr + 4);
                cur_track->stss_count = count;
                if (count > 0 && count < 10000) {
                    cur_track->stss_table = (uint32_t*)bospectra_mem_alloc(count * sizeof(uint32_t), "stss_table");
                    if (cur_track->stss_table) {
                        uint32_t table_bytes = count * 4;
                        uint8_t* raw_ss = (uint8_t*)bospectra_mem_alloc(table_bytes, "raw_ss");
                        if (raw_ss) {
                            if (bospectra_file_read(ctx->file_id, raw_ss, table_bytes, &read_bytes) == BOSPECTRA_SUCCESS) {
                                for (uint32_t si = 0; si < count; si++) {
                                    cur_track->stss_table[si] = read_u32_be(raw_ss + si * 4);
                                }
                            }
                            bospectra_mem_free(raw_ss);
                        }
                    }
                }
            }
        }

        offset += box_size;
    }
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t mp4_demux_open(bospectra_file_id_t file_id, MP4_DemuxContext** out_ctx) {
    if (!out_ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint64_t file_size = 0;
    if (bospectra_file_get_size(file_id, &file_size) != BOSPECTRA_SUCCESS || file_size == 0) {
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }

    MP4_DemuxContext* ctx = (MP4_DemuxContext*)bospectra_mem_alloc(sizeof(MP4_DemuxContext), "MP4DemuxCtx");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;
    memset(ctx, 0, sizeof(MP4_DemuxContext));
    ctx->file_id = file_id;
    ctx->file_size = file_size;
    ctx->video_track_idx = -1;
    ctx->audio_track_idx = -1;

    bospectra_error_t err = parse_boxes_recursive(ctx, 0, file_size, 0);
    if (err != BOSPECTRA_SUCCESS || ctx->track_count == 0) {
        mp4_demux_close(ctx);
        return BOSPECTRA_ERR_UNSUPPORTED_FORMAT;
    }

    for (uint32_t i = 0; i < ctx->track_count; i++) {
        if (ctx->tracks[i].type == BOSPECTRA_STREAM_VIDEO && ctx->video_track_idx < 0) {
            ctx->video_track_idx = (int32_t)i;
        } else if (ctx->tracks[i].type == BOSPECTRA_STREAM_AUDIO && ctx->audio_track_idx < 0) {
            ctx->audio_track_idx = (int32_t)i;
        }
    }

    *out_ctx = ctx;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t mp4_demux_get_sample_info(MP4_DemuxContext* ctx, uint32_t track_idx, uint32_t sample_idx,
                                           uint64_t* out_file_offset, uint32_t* out_size,
                                           uint64_t* out_pts_us, bool* out_is_keyframe) {
    if (!ctx || track_idx >= ctx->track_count) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    MP4_DemuxTrack* trk = &ctx->tracks[track_idx];
    if (sample_idx >= trk->sample_count) return BOSPECTRA_ERR_BUFFER_UNDERFLOW;

    if (!trk->sample_sizes || !trk->chunk_offsets || !trk->stsc_table) {
        return BOSPECTRA_ERR_STATE_INVALID;
    }

    uint32_t sample_size = trk->sample_sizes[sample_idx];
    if (out_size) *out_size = sample_size;

    /* Find which chunk contains sample_idx */
    uint32_t cur_chunk = 1;
    uint32_t cur_sample = 0;
    uint32_t stsc_idx = 0;
    uint32_t samples_in_cur_chunk = (trk->stsc_count > 0) ? trk->stsc_table[0].samples_per_chunk : 1;

    while (cur_chunk <= trk->chunk_count) {
        if (stsc_idx + 1 < trk->stsc_count && cur_chunk >= trk->stsc_table[stsc_idx + 1].first_chunk) {
            stsc_idx++;
            samples_in_cur_chunk = trk->stsc_table[stsc_idx].samples_per_chunk;
        }

        if (sample_idx < cur_sample + samples_in_cur_chunk) {
            /* Sample is in this chunk! */
            uint64_t chunk_base = trk->chunk_offsets[cur_chunk - 1];
            uint64_t sample_offset_in_chunk = 0;
            for (uint32_t s = cur_sample; s < sample_idx; s++) {
                sample_offset_in_chunk += trk->sample_sizes[s];
            }
            if (out_file_offset) *out_file_offset = chunk_base + sample_offset_in_chunk;
            break;
        }

        cur_sample += samples_in_cur_chunk;
        cur_chunk++;
    }

    /* PTS Calculation from stts */
    uint64_t pts_ticks = 0;
    uint32_t remaining = sample_idx;
    for (uint32_t i = 0; i < trk->stts_count && remaining > 0; i++) {
        uint32_t cnt = trk->stts_table[i].sample_count;
        uint32_t take = (remaining < cnt) ? remaining : cnt;
        pts_ticks += (uint64_t)take * trk->stts_table[i].sample_delta;
        remaining -= take;
    }
    uint32_t ts = trk->timescale ? trk->timescale : 30;
    if (out_pts_us) *out_pts_us = (pts_ticks * 1000000ULL) / ts;

    /* Keyframe check from stss */
    bool is_sync = false;
    if (trk->stss_table && trk->stss_count > 0) {
        uint32_t s_num = sample_idx + 1; /* 1-based */
        for (uint32_t i = 0; i < trk->stss_count; i++) {
            if (trk->stss_table[i] == s_num) {
                is_sync = true;
                break;
            }
        }
    } else {
        is_sync = (sample_idx == 0); /* Default to sample 0 if no sync table */
    }
    if (out_is_keyframe) *out_is_keyframe = is_sync;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t mp4_demux_read_sample(MP4_DemuxContext* ctx, uint32_t track_idx, uint32_t sample_idx,
                                        void* buffer, uint32_t buffer_size, uint32_t* out_bytes_read) {
    if (!ctx || !buffer || !out_bytes_read) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint64_t file_offset = 0;
    uint32_t sample_size = 0;
    bospectra_error_t err = mp4_demux_get_sample_info(ctx, track_idx, sample_idx, &file_offset, &sample_size, NULL, NULL);
    if (err != BOSPECTRA_SUCCESS) return err;

    if (buffer_size < sample_size) return BOSPECTRA_ERR_BUFFER_OVERFLOW;

    if (bospectra_file_seek(ctx->file_id, file_offset) != BOSPECTRA_SUCCESS) {
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }

    uint32_t read = 0;
    if (bospectra_file_read(ctx->file_id, buffer, sample_size, &read) != BOSPECTRA_SUCCESS || read != sample_size) {
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }

    *out_bytes_read = read;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t mp4_demux_close(MP4_DemuxContext* ctx) {
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < ctx->track_count; i++) {
        MP4_DemuxTrack* trk = &ctx->tracks[i];
        if (trk->sample_sizes) bospectra_mem_free(trk->sample_sizes);
        if (trk->chunk_offsets) bospectra_mem_free(trk->chunk_offsets);
        if (trk->stsc_table) bospectra_mem_free(trk->stsc_table);
        if (trk->stts_table) bospectra_mem_free(trk->stts_table);
        if (trk->stss_table) bospectra_mem_free(trk->stss_table);
    }
    bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}
