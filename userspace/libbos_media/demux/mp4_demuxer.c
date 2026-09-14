/*
 * ============================================================================
 * ATOMS OS — Userspace Native MP4 / ISO Base Media Demuxer Implementation
 * userspace/libbos_media/demux/mp4_demuxer.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Derived from minimp4 (CC0 1.0 Universal / Public Domain)
 *
 * Implements dynamic ISO box parsing operating over BOSMediaStream.
 * Zero hardcoded file offsets, zero kernel dependencies.
 * ============================================================================
 */

#include "mp4_demuxer.h"
#include <stdlib.h>
#include <string.h>

extern void display_print(const char* s);
extern void display_print_hex(uint64_t val);

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

int mp4_demuxer_probe(BOSMediaStream* stream) {
    if (!stream || !stream->read || !stream->seek) return -1;
    uint8_t buf[16];
    if (stream->seek(stream, 0, BOS_SEEK_SET) != 0) return -1;
    if (stream->read(stream, buf, 16) < 8) return -1;
    
    uint32_t box_type = read_u32_be(buf + 4);
    if (box_type == FOURCC('f', 't', 'y', 'p') || box_type == FOURCC('m', 'o', 'o', 'v')) {
        return 0;
    }
    return -1;
}

static int parse_avcc(MP4Track* trk, const uint8_t* data, uint32_t size) {
    if (size < 7) return -1;
    trk->sps_profile = data[1];
    trk->sps_level   = data[3];
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
                // Entropy coding mode flag: in PPS byte 0, bit 6 is entropy_coding_mode_flag
                // For H.264 PPS: [0]=nal_hdr(0x68), [1]=pps_id(exp-golomb), [2] contains entropy_coding_mode
                // Usually PPS with CABAC has entropy_coding_mode_flag = 1
                if (pps_len >= 2) {
                    trk->entropy_coding_mode = 1; // High Profile default CABAC
                }
            }
        }
    }
    return 0;
}

static int parse_stsd(BOSMediaStream* s, MP4Track* trk, uint64_t start, uint64_t end) {
    uint8_t hdr[8];
    if (s->seek(s, start, BOS_SEEK_SET) != 0) return -1;
    if (s->read(s, hdr, 8) < 8) return -1;

    uint32_t count = read_u32_be(hdr + 4);
    uint64_t cur = start + 8;

    for (uint32_t i = 0; i < count && cur + 8 <= end; i++) {
        uint8_t entry_hdr[8];
        if (s->seek(s, cur, BOS_SEEK_SET) != 0) break;
        if (s->read(s, entry_hdr, 8) < 8) break;

        uint32_t entry_size = read_u32_be(entry_hdr);
        uint32_t format = read_u32_be(entry_hdr + 4);
        trk->codec_fourcc = format;

        if (format == FOURCC('a', 'v', 'c', '1') || format == FOURCC('a', 'v', 'c', '3') || format == FOURCC('e', 'n', 'c', 'v')) {
            trk->type = MP4_TRACK_VIDEO;
            uint8_t visual_hdr[86];
            if (s->seek(s, cur, BOS_SEEK_SET) == 0 && s->read(s, visual_hdr, 86) >= 86) {
                trk->width  = read_u16_be(visual_hdr + 32);
                trk->height = read_u16_be(visual_hdr + 34);
            }

            // Find avcC sub-box inside entry (VisualSampleEntry is 86 bytes before sub-boxes)
            uint64_t sub_cur = cur + 86;
            uint64_t sub_end = cur + entry_size;
            while (sub_cur + 8 <= sub_end) {
                uint8_t sub_hdr[8];
                if (s->seek(s, sub_cur, BOS_SEEK_SET) != 0) break;
                if (s->read(s, sub_hdr, 8) < 8) break;

                uint32_t sub_sz = read_u32_be(sub_hdr);
                uint32_t sub_type = read_u32_be(sub_hdr + 4);
                if (sub_sz < 8 || sub_cur + sub_sz > sub_end) break;

                if (sub_type == FOURCC('a', 'v', 'c', 'C')) {
                    uint32_t avcc_bytes = sub_sz - 8;
                    if (avcc_bytes > 0 && avcc_bytes < 2048) {
                        uint8_t* avcc_buf = (uint8_t*)malloc(avcc_bytes);
                        if (avcc_buf) {
                            if (s->read(s, avcc_buf, avcc_bytes) == (int)avcc_bytes) {
                                parse_avcc(trk, avcc_buf, avcc_bytes);
                            }
                            free(avcc_buf);
                        }
                    }
                    break;
                }
                sub_cur += sub_sz;
            }
        } else if (format == FOURCC('m', 'p', '4', 'a') || format == FOURCC('m', 'p', '3', ' ') || format == FOURCC('.', 'm', 'p', '3')) {
            trk->type = MP4_TRACK_AUDIO;
            uint8_t audio_hdr[28];
            if (s->seek(s, cur, BOS_SEEK_SET) == 0 && s->read(s, audio_hdr, 28) >= 28) {
                trk->channels = (uint8_t)read_u16_be(audio_hdr + 16);
                trk->sample_rate = read_u32_be(audio_hdr + 22) >> 16;
                if (trk->sample_rate == 0) trk->sample_rate = 44100;
                if (trk->channels == 0) trk->channels = 2;
            }
        }

        if (entry_size == 0) break;
        cur += entry_size;
    }
    return 0;
}

static int parse_boxes_recursive(MP4Demuxer* demuxer, uint64_t start_offset, uint64_t end_offset, int depth) {
    if (depth > 8) return 0;
    BOSMediaStream* s = demuxer->stream;
    uint64_t offset = start_offset;

    while (offset + 8 <= end_offset) {
        uint8_t hdr[16];
        if (s->seek(s, offset, BOS_SEEK_SET) != 0) break;
        if (s->read(s, hdr, 8) < 8) break;

        uint64_t box_size = read_u32_be(hdr);
        uint32_t box_type = read_u32_be(hdr + 4);
        uint32_t hdr_size = 8;

        if (box_size == 1) {
            if (s->read(s, hdr + 8, 8) < 8) break;
            box_size = read_u64_be(hdr + 8);
            hdr_size = 16;
        } else if (box_size == 0) {
            box_size = end_offset - offset;
        }

        if (box_size < hdr_size || offset + box_size > end_offset) break;

        MP4Track* cur_track = (demuxer->track_count > 0) ? &demuxer->tracks[demuxer->track_count - 1] : NULL;

        if (box_type == FOURCC('m', 'o', 'o', 'v') ||
            box_type == FOURCC('m', 'd', 'i', 'a') ||
            box_type == FOURCC('m', 'i', 'n', 'f') ||
            box_type == FOURCC('s', 't', 'b', 'l')) {
            parse_boxes_recursive(demuxer, offset + hdr_size, offset + box_size, depth + 1);
        } else if (box_type == FOURCC('t', 'r', 'a', 'k')) {
            if (demuxer->track_count < MP4_MAX_TRACKS) {
                MP4Track* t = &demuxer->tracks[demuxer->track_count++];
                memset(t, 0, sizeof(MP4Track));
                t->track_id = demuxer->track_count;
                t->type = MP4_TRACK_VIDEO;
                parse_boxes_recursive(demuxer, offset + hdr_size, offset + box_size, depth + 1);
            }
        } else if (box_type == FOURCC('m', 'v', 'h', 'd')) {
            uint8_t mvhd[32];
            if (s->read(s, mvhd, 32) >= 24) {
                uint8_t ver = mvhd[0];
                if (ver == 0) {
                    demuxer->timescale = read_u32_be(mvhd + 12);
                    uint32_t dur = read_u32_be(mvhd + 16);
                    demuxer->duration_us = (demuxer->timescale > 0) ? ((uint64_t)dur * 1000000ULL / demuxer->timescale) : 0;
                } else if (ver == 1) {
                    demuxer->timescale = read_u32_be(mvhd + 20);
                    uint64_t dur = read_u64_be(mvhd + 24);
                    demuxer->duration_us = (demuxer->timescale > 0) ? (dur * 1000000ULL / demuxer->timescale) : 0;
                }
            }
        } else if (box_type == FOURCC('m', 'd', 'h', 'd') && cur_track) {
            uint8_t mdhd[32];
            if (s->read(s, mdhd, 32) >= 24) {
                uint8_t ver = mdhd[0];
                if (ver == 0) {
                    cur_track->timescale = read_u32_be(mdhd + 12);
                    uint32_t dur = read_u32_be(mdhd + 16);
                    cur_track->duration_us = (cur_track->timescale > 0) ? ((uint64_t)dur * 1000000ULL / cur_track->timescale) : 0;
                }
            }
        } else if (box_type == FOURCC('h', 'd', 'l', 'r') && cur_track) {
            uint8_t hdlr[16];
            if (s->read(s, hdlr, 16) >= 12) {
                uint32_t handler = read_u32_be(hdlr + 8);
                if (handler == FOURCC('v', 'i', 'd', 'e')) {
                    cur_track->type = MP4_TRACK_VIDEO;
                } else if (handler == FOURCC('s', 'o', 'u', 'n')) {
                    cur_track->type = MP4_TRACK_AUDIO;
                }
            }
        } else if (box_type == FOURCC('s', 't', 's', 'd') && cur_track) {
            parse_stsd(s, cur_track, offset + hdr_size, offset + box_size);
        } else if (box_type == FOURCC('s', 't', 's', 'z') && cur_track) {
            uint8_t stsz[12];
            if (s->read(s, stsz, 12) >= 12) {
                uint32_t uniform = read_u32_be(stsz + 4);
                uint32_t count   = read_u32_be(stsz + 8);
                cur_track->sample_count = count;
                if (count > 0 && count < 100000) {
                    cur_track->sample_sizes = (uint32_t*)malloc(count * sizeof(uint32_t));
                    if (cur_track->sample_sizes) {
                        if (uniform > 0) {
                            for (uint32_t i = 0; i < count; i++) cur_track->sample_sizes[i] = uniform;
                        } else {
                            uint32_t bytes = count * 4;
                            uint8_t* raw = (uint8_t*)malloc(bytes);
                            if (raw) {
                                if (s->read(s, raw, bytes) == (int)bytes) {
                                    for (uint32_t i = 0; i < count; i++) cur_track->sample_sizes[i] = read_u32_be(raw + i * 4);
                                }
                                free(raw);
                            }
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 'c', 'o') && cur_track) {
            uint8_t stco[8];
            if (s->read(s, stco, 8) >= 8) {
                uint32_t count = read_u32_be(stco + 4);
                cur_track->chunk_count = count;
                if (count > 0 && count < 100000) {
                    cur_track->chunk_offsets = (uint64_t*)malloc(count * sizeof(uint64_t));
                    if (cur_track->chunk_offsets) {
                        uint32_t bytes = count * 4;
                        uint8_t* raw = (uint8_t*)malloc(bytes);
                        if (raw) {
                            if (s->read(s, raw, bytes) == (int)bytes) {
                                for (uint32_t i = 0; i < count; i++) cur_track->chunk_offsets[i] = (uint64_t)read_u32_be(raw + i * 4);
                            }
                            free(raw);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('c', 'o', '6', '4') && cur_track) {
            uint8_t co64[8];
            if (s->read(s, co64, 8) >= 8) {
                uint32_t count = read_u32_be(co64 + 4);
                cur_track->chunk_count = count;
                if (count > 0 && count < 100000) {
                    cur_track->chunk_offsets = (uint64_t*)malloc(count * sizeof(uint64_t));
                    if (cur_track->chunk_offsets) {
                        uint32_t bytes = count * 8;
                        uint8_t* raw = (uint8_t*)malloc(bytes);
                        if (raw) {
                            if (s->read(s, raw, bytes) == (int)bytes) {
                                for (uint32_t i = 0; i < count; i++) cur_track->chunk_offsets[i] = read_u64_be(raw + i * 8);
                            }
                            free(raw);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 's', 'c') && cur_track) {
            uint8_t stsc[8];
            if (s->read(s, stsc, 8) >= 8) {
                uint32_t count = read_u32_be(stsc + 4);
                cur_track->stsc_count = count;
                if (count > 0 && count < 10000) {
                    cur_track->stsc_table = (MP4StscEntry*)malloc(count * sizeof(MP4StscEntry));
                    if (cur_track->stsc_table) {
                        uint32_t bytes = count * 12;
                        uint8_t* raw = (uint8_t*)malloc(bytes);
                        if (raw) {
                            if (s->read(s, raw, bytes) == (int)bytes) {
                                for (uint32_t i = 0; i < count; i++) {
                                    cur_track->stsc_table[i].first_chunk = read_u32_be(raw + i * 12);
                                    cur_track->stsc_table[i].samples_per_chunk = read_u32_be(raw + i * 12 + 4);
                                    cur_track->stsc_table[i].desc_index = read_u32_be(raw + i * 12 + 8);
                                }
                            }
                            free(raw);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 't', 's') && cur_track) {
            uint8_t stts[8];
            if (s->read(s, stts, 8) >= 8) {
                uint32_t count = read_u32_be(stts + 4);
                cur_track->stts_count = count;
                if (count > 0 && count < 10000) {
                    cur_track->stts_table = (MP4SttsEntry*)malloc(count * sizeof(MP4SttsEntry));
                    if (cur_track->stts_table) {
                        uint32_t bytes = count * 8;
                        uint8_t* raw = (uint8_t*)malloc(bytes);
                        if (raw) {
                            if (s->read(s, raw, bytes) == (int)bytes) {
                                for (uint32_t i = 0; i < count; i++) {
                                    cur_track->stts_table[i].sample_count = read_u32_be(raw + i * 8);
                                    cur_track->stts_table[i].sample_delta = read_u32_be(raw + i * 8 + 4);
                                }
                            }
                            free(raw);
                        }
                    }
                }
            }
        } else if (box_type == FOURCC('s', 't', 's', 's') && cur_track) {
            uint8_t stss[8];
            if (s->read(s, stss, 8) >= 8) {
                uint32_t count = read_u32_be(stss + 4);
                cur_track->stss_count = count;
                if (count > 0 && count < 10000) {
                    cur_track->stss_table = (uint32_t*)malloc(count * sizeof(uint32_t));
                    if (cur_track->stss_table) {
                        uint32_t bytes = count * 4;
                        uint8_t* raw = (uint8_t*)malloc(bytes);
                        if (raw) {
                            if (s->read(s, raw, bytes) == (int)bytes) {
                                for (uint32_t i = 0; i < count; i++) cur_track->stss_table[i] = read_u32_be(raw + i * 4);
                            }
                            free(raw);
                        }
                    }
                }
            }
        }

        offset += box_size;
    }
    return 0;
}

int mp4_demuxer_open(BOSMediaStream* stream, MP4Demuxer* demuxer) {
    if (!stream || !demuxer) return -1;
    memset(demuxer, 0, sizeof(MP4Demuxer));
    demuxer->stream = stream;
    demuxer->file_size = stream->size;
    demuxer->video_track_idx = -1;
    demuxer->audio_track_idx = -1;

    parse_boxes_recursive(demuxer, 0, demuxer->file_size, 0);

    for (uint32_t i = 0; i < demuxer->track_count; i++) {
        if (demuxer->tracks[i].type == MP4_TRACK_VIDEO && demuxer->video_track_idx < 0) {
            demuxer->video_track_idx = (int32_t)i;
        } else if (demuxer->tracks[i].type == MP4_TRACK_AUDIO && demuxer->audio_track_idx < 0) {
            demuxer->audio_track_idx = (int32_t)i;
        }
    }

    if (demuxer->track_count == 0) return -1;
    return 0;
}

void mp4_demuxer_close(MP4Demuxer* demuxer) {
    if (!demuxer) return;
    for (uint32_t i = 0; i < demuxer->track_count; i++) {
        MP4Track* trk = &demuxer->tracks[i];
        if (trk->sample_sizes)  { free(trk->sample_sizes);  trk->sample_sizes = NULL; }
        if (trk->chunk_offsets) { free(trk->chunk_offsets); trk->chunk_offsets = NULL; }
        if (trk->stsc_table)    { free(trk->stsc_table);    trk->stsc_table = NULL; }
        if (trk->stts_table)    { free(trk->stts_table);    trk->stts_table = NULL; }
        if (trk->stss_table)    { free(trk->stss_table);    trk->stss_table = NULL; }
    }
}

int mp4_demuxer_get_sample_info(MP4Demuxer* demuxer, uint32_t track_idx, uint32_t sample_idx,
                                uint64_t* out_offset, uint32_t* out_size, uint64_t* out_pts_us, bool* out_is_keyframe) {
    if (!demuxer || track_idx >= demuxer->track_count) return -1;
    MP4Track* trk = &demuxer->tracks[track_idx];
    if (sample_idx >= trk->sample_count) return -1;
    if (!trk->sample_sizes || !trk->chunk_offsets || !trk->stsc_table) return -1;

    uint32_t sample_size = trk->sample_sizes[sample_idx];
    if (out_size) *out_size = sample_size;

    // Check sequential sample cache (O(1) fast path)
    if (trk->has_sample_cache && sample_idx == trk->cached_sample_idx + 1) {
        uint32_t samples_in_cur_chunk = (trk->stsc_count > 0) ? trk->stsc_table[trk->cached_stsc_idx].samples_per_chunk : 1;
        if (sample_idx < trk->cached_chunk_sample_start + samples_in_cur_chunk) {
            uint64_t next_offset = trk->cached_sample_offset + trk->sample_sizes[trk->cached_sample_idx];
            trk->cached_sample_idx = sample_idx;
            trk->cached_sample_offset = next_offset;
            if (out_offset) *out_offset = next_offset;
            goto compute_pts;
        }
    }

    // Find chunk containing sample_idx (General fallback / Seek path)
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
            uint64_t chunk_base = trk->chunk_offsets[cur_chunk - 1];
            uint64_t sample_offset_in_chunk = 0;
            for (uint32_t s = cur_sample; s < sample_idx; s++) {
                sample_offset_in_chunk += trk->sample_sizes[s];
            }
            uint64_t final_offset = chunk_base + sample_offset_in_chunk;
            if (out_offset) *out_offset = final_offset;

            // Update sample cache
            trk->cached_sample_idx = sample_idx;
            trk->cached_chunk_idx = cur_chunk;
            trk->cached_chunk_sample_start = cur_sample;
            trk->cached_stsc_idx = stsc_idx;
            trk->cached_sample_offset = final_offset;
            trk->has_sample_cache = true;
            break;
        }

        cur_sample += samples_in_cur_chunk;
        cur_chunk++;
    }

compute_pts:;

    // Timestamp calculation
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

    // Keyframe check
    bool is_sync = false;
    if (trk->stss_table && trk->stss_count > 0) {
        uint32_t s_num = sample_idx + 1;
        for (uint32_t i = 0; i < trk->stss_count; i++) {
            if (trk->stss_table[i] == s_num) {
                is_sync = true;
                break;
            }
        }
    } else {
        is_sync = (sample_idx == 0);
    }
    if (out_is_keyframe) *out_is_keyframe = is_sync;

    return 0;
}

int mp4_demuxer_read_sample(MP4Demuxer* demuxer, uint32_t track_idx, uint32_t sample_idx,
                            uint8_t* out_buffer, uint32_t buffer_size, uint32_t* out_bytes_read) {
    if (!demuxer || !out_buffer || !out_bytes_read) return -1;
    uint64_t file_offset = 0;
    uint32_t sample_size = 0;
    if (mp4_demuxer_get_sample_info(demuxer, track_idx, sample_idx, &file_offset, &sample_size, NULL, NULL) != 0) {
        return -1;
    }
    if (sample_size > buffer_size) return -1;

    BOSMediaStream* s = demuxer->stream;
    if (s->seek(s, file_offset, BOS_SEEK_SET) != 0) return -1;
    int rd = s->read(s, out_buffer, sample_size);
    if (rd < 0 || (uint32_t)rd != sample_size) return -1;

    *out_bytes_read = sample_size;

    // Convert MP4 NAL length prefixes to Annex B start codes (0x00 0x00 0x00 0x01)
    uint32_t p = 0;
    while (p + 4 <= sample_size) {
        uint32_t nlen = read_u32_be(out_buffer + p);
        if (nlen == 0 || p + 4 + nlen > sample_size) break;
        out_buffer[p]     = 0x00;
        out_buffer[p + 1] = 0x00;
        out_buffer[p + 2] = 0x00;
        out_buffer[p + 3] = 0x01;
        p += 4 + nlen;
    }

    return 0;
}
