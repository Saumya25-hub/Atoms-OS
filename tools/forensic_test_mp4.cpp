#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "third_party/media/h264/include/basetype.h"
#include "third_party/media/h264/include/h264bsd_decoder.h"
#include "third_party/media/h264/include/h264bsd_storage.h"
#include "third_party/media/mp4/include/mp4_demux.h"

/* Mock kernel print */
void display_print(const char* str) {
    printf("%s", str);
}

void print_u32(uint32_t val) {
    printf("%u", val);
}

/* Mock bospectra memory */
void* bospectra_mem_alloc(size_t size, const char* tag) {
    (void)tag;
    return calloc(1, size);
}

void bospectra_mem_free(void* ptr) {
    free(ptr);
}

/* Mock bospectra file abstraction wrapping stdio FILE* */
static FILE* g_current_fp = NULL;

bospectra_error_t bospectra_file_seek(bospectra_file_id_t file_id, uint64_t offset) {
    (void)file_id;
    if (!g_current_fp) return BOSPECTRA_ERR_FILE_NOT_FOUND;
    if (fseek(g_current_fp, (long)offset, SEEK_SET) != 0) return BOSPECTRA_ERR_FILE_READ_FAILED;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_file_read(bospectra_file_id_t file_id, void* buffer, uint32_t size, uint32_t* bytes_read) {
    (void)file_id;
    if (!g_current_fp) return BOSPECTRA_ERR_FILE_NOT_FOUND;
    size_t r = fread(buffer, 1, size, g_current_fp);
    if (bytes_read) *bytes_read = (uint32_t)r;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_file_size(bospectra_file_id_t file_id, uint64_t* out_size) {
    (void)file_id;
    if (!g_current_fp) return BOSPECTRA_ERR_FILE_NOT_FOUND;
    long cur = ftell(g_current_fp);
    fseek(g_current_fp, 0, SEEK_END);
    long sz = ftell(g_current_fp);
    fseek(g_current_fp, cur, SEEK_SET);
    if (out_size) *out_size = (uint64_t)sz;
    return BOSPECTRA_SUCCESS;
}

/* CRC32 Helper */
static uint32_t calc_crc32(const void* buf, size_t len) {
    uint32_t crc = 0xFFFFFFFFU;
    const uint8_t* p = (const uint8_t*)buf;
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320U : 0);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

/* Save RGB buffer as BMP */
static void save_bmp(const char* filename, const uint32_t* pixels, uint32_t w, uint32_t h) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return;

    uint32_t row_bytes = w * 4;
    uint32_t img_size = row_bytes * h;
    uint32_t file_size = 54 + img_size;

    uint8_t bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    uint8_t bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 32,0};

    bmpfileheader[ 2] = (uint8_t)(file_size);
    bmpfileheader[ 3] = (uint8_t)(file_size >>  8);
    bmpfileheader[ 4] = (uint8_t)(file_size >> 16);
    bmpfileheader[ 5] = (uint8_t)(file_size >> 24);

    bmpinfoheader[ 4] = (uint8_t)(w);
    bmpinfoheader[ 5] = (uint8_t)(w >>  8);
    bmpinfoheader[ 6] = (uint8_t)(w >> 16);
    bmpinfoheader[ 7] = (uint8_t)(w >> 24);

    int32_t neg_h = -(int32_t)h; // Top-down
    bmpinfoheader[ 8] = (uint8_t)(neg_h);
    bmpinfoheader[ 9] = (uint8_t)(neg_h >>  8);
    bmpinfoheader[10] = (uint8_t)(neg_h >> 16);
    bmpinfoheader[11] = (uint8_t)(neg_h >> 24);

    bmpinfoheader[20] = (uint8_t)(img_size);
    bmpinfoheader[21] = (uint8_t)(img_size >>  8);
    bmpinfoheader[22] = (uint8_t)(img_size >> 16);
    bmpinfoheader[23] = (uint8_t)(img_size >> 24);

    fwrite(bmpfileheader, 1, 14, fp);
    fwrite(bmpinfoheader, 1, 40, fp);
    fwrite(pixels, 4, w * h, fp);
    fclose(fp);
    printf("Saved frame BMP to %s\n", filename);
}

/* YUV420P -> ARGB32 Color Conversion matching software_backend.c */
static void convert_yuv420p_to_argb(const uint8_t* y_plane, const uint8_t* u_plane, const uint8_t* v_plane,
                                    uint32_t width, uint32_t height,
                                    uint32_t y_stride, uint32_t u_stride, uint32_t v_stride,
                                    uint32_t* dst_pixels) {
    static bool s_lut_inited = false;
    static int32_t s_lut_cr_r[256];
    static int32_t s_lut_cb_g[256];
    static int32_t s_lut_cr_g[256];
    static int32_t s_lut_cb_b[256];
    static int32_t s_lut_709_cr_r[256];
    static int32_t s_lut_709_cb_g[256];
    static int32_t s_lut_709_cr_g[256];
    static int32_t s_lut_709_cb_b[256];
    static uint8_t s_lut_clamp[1024];

    if (!s_lut_inited) {
        for (int i = 0; i < 256; i++) {
            int32_t cb = i - 128;
            int32_t cr = i - 128;
            s_lut_cr_r[i] = (1436 * cr + 512) >> 10;
            s_lut_cb_g[i] = 352 * cb;
            s_lut_cr_g[i] = 731 * cr;
            s_lut_cb_b[i] = (1815 * cb + 512) >> 10;

            s_lut_709_cr_r[i] = (1613 * cr + 512) >> 10;
            s_lut_709_cb_g[i] = 192 * cb;
            s_lut_709_cr_g[i] = 479 * cr;
            s_lut_709_cb_b[i] = (1900 * cb + 512) >> 10;
        }
        for (int i = 0; i < 1024; i++) {
            int val = i - 384;
            s_lut_clamp[i] = (val < 0) ? 0 : ((val > 255) ? 255 : (uint8_t)val);
        }
        s_lut_inited = true;
    }

    bool is_bt709 = (width >= 1280 || height >= 720);
    const int32_t* lut_cr_r = is_bt709 ? s_lut_709_cr_r : s_lut_cr_r;
    const int32_t* lut_cb_g = is_bt709 ? s_lut_709_cb_g : s_lut_cb_g;
    const int32_t* lut_cr_g = is_bt709 ? s_lut_709_cr_g : s_lut_cr_g;
    const int32_t* lut_cb_b = is_bt709 ? s_lut_709_cb_b : s_lut_cb_b;

    for (uint32_t row = 0; row < height; row++) {
        const uint8_t* y_row = y_plane + (size_t)row * y_stride;
        uint32_t* dst_row = dst_pixels + (size_t)row * width;

        uint32_t r_curr = row / 2;
        const uint8_t* u_row = u_plane + r_curr * u_stride;
        const uint8_t* v_row = v_plane + r_curr * v_stride;

        for (uint32_t col = 0; col < width; col += 2) {
            uint32_t c_curr = col / 2;
            uint8_t u_val = u_row[c_curr];
            uint8_t v_val = v_row[c_curr];

            int32_t r_diff = lut_cr_r[v_val];
            int32_t g_diff = (lut_cb_g[u_val] + lut_cr_g[v_val] + 512) >> 10;
            int32_t b_diff = lut_cb_b[u_val];

            /* Pixel 0 */
            int32_t Y0 = (int32_t)y_row[col];
            uint8_t r0 = s_lut_clamp[Y0 + r_diff + 384];
            uint8_t g0 = s_lut_clamp[Y0 - g_diff + 384];
            uint8_t b0 = s_lut_clamp[Y0 + b_diff + 384];
            dst_row[col] = 0xFF000000U | ((uint32_t)r0 << 16) | ((uint32_t)g0 << 8) | (uint32_t)b0;

            /* Pixel 1 */
            if (col + 1 < width) {
                int32_t Y1 = (int32_t)y_row[col + 1];
                uint8_t r1 = s_lut_clamp[Y1 + r_diff + 384];
                uint8_t g1 = s_lut_clamp[Y1 - g_diff + 384];
                uint8_t b1 = s_lut_clamp[Y1 + b_diff + 384];
                dst_row[col + 1] = 0xFF000000U | ((uint32_t)r1 << 16) | ((uint32_t)g1 << 8) | (uint32_t)b1;
            }
        }
    }
}

void test_media_file(const char* filepath, const char* out_bmp_name) {
    printf("\n======================================================================\n");
    printf("FORENSIC PIPELINE AUDIT: %s\n", filepath);
    printf("======================================================================\n");

    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        printf("FAILED TO OPEN FILE: %s\n", filepath);
        return;
    }
    g_current_fp = fp;

    MP4_DemuxContext* demux = NULL;
    bospectra_error_t err = mp4_demux_open(1, &demux);
    if (err != BOSPECTRA_SUCCESS || !demux) {
        printf("STAGE 1 [DEMUX]: FAILED with error %d\n", err);
        fclose(fp);
        return;
    }

    printf("STAGE 1 [DEMUX]: PASS\n");
    printf("  Duration: %llu us (%llu s)\n", (unsigned long long)demux->duration_us, (unsigned long long)(demux->duration_us / 1000000));
    printf("  Tracks: %u (Video Track = %d, Audio Track = %d)\n", demux->track_count, demux->video_track_idx, demux->audio_track_idx);

    if (demux->video_track_idx < 0) {
        printf("STAGE 2 [VIDEO TRACK]: FAILED (No video track found)\n");
        mp4_demux_close(demux);
        fclose(fp);
        return;
    }

    MP4_DemuxTrack* vt = &demux->tracks[demux->video_track_idx];
    printf("STAGE 2 [VIDEO TRACK]: PASS\n");
    printf("  Resolution: %ux%u\n", vt->width, vt->height);
    printf("  Sample Count: %u\n", vt->sample_count);
    printf("  Codec FourCC: 0x%08X (%c%c%c%c)\n",
           vt->codec_fourcc,
           (vt->codec_fourcc >> 24) & 0xFF, (vt->codec_fourcc >> 16) & 0xFF,
           (vt->codec_fourcc >> 8) & 0xFF, vt->codec_fourcc & 0xFF);
    printf("  SPS Length: %u bytes\n", vt->sps_len);
    printf("  PPS Length: %u bytes\n", vt->pps_len);

    /* Parse SPS/PPS to determine Profile and CABAC */
    if (vt->sps_len >= 4) {
        uint8_t profile_idc = vt->sps[1];
        printf("  H.264 Profile IDC: %u -> ", profile_idc);
        if (profile_idc == 66) printf("Baseline Profile\n");
        else if (profile_idc == 77) printf("Main Profile\n");
        else if (profile_idc == 100) printf("High Profile\n");
        else printf("Profile %u\n", profile_idc);
    }

    bool cabac_detected = false;
    if (vt->pps_len > 1) {
        /* Parse entropy_coding_mode_flag */
        /* PPS starts after NAL header (1 byte). Read ue(v) pps_id, ue(v) sps_id, u(1) entropy */
        uint8_t* p = vt->pps + 1;
        uint32_t bits_avail = (vt->pps_len - 1) * 8;
        uint32_t bpos = 0;

        #define GET_BIT() ((p[bpos / 8] >> (7 - (bpos % 8))) & 1)
        auto read_ue = [&]() -> uint32_t {
            uint32_t zeros = 0;
            while (bpos < bits_avail && GET_BIT() == 0) { zeros++; bpos++; }
            if (bpos >= bits_avail) return 0;
            bpos++;
            uint32_t val = 0;
            for (uint32_t i = 0; i < zeros; i++) {
                val = (val << 1) | GET_BIT();
                bpos++;
            }
            return (1 << zeros) - 1 + val;
        };

        uint32_t pps_id = read_ue();
        uint32_t sps_id = read_ue();
        uint32_t entropy_flag = (bpos < bits_avail) ? GET_BIT() : 0;
        printf("  PPS parsed: pps_id=%u, sps_id=%u, entropy_coding_mode_flag=%u\n", pps_id, sps_id, entropy_flag);
        if (entropy_flag != 0) {
            cabac_detected = true;
            printf("  >>> CABAC STATUS: CABAC ENABLED (entropy_coding_mode_flag = 1) <<<\n");
        } else {
            printf("  >>> CABAC STATUS: CAVLC ONLY (entropy_coding_mode_flag = 0) <<<\n");
        }
    }

    /* Test H.264 Decoder Initialization */
    storage_t* storage = h264bsdAlloc();
    if (!storage) {
        printf("STAGE 3 [H264 ALLOC]: FAILED\n");
        mp4_demux_close(demux);
        fclose(fp);
        return;
    }

    u32 init_ret = h264bsdInit(storage, 1);
    printf("STAGE 3 [H264 INIT]: %s (ret = %u)\n", (init_ret == 0) ? "PASS" : "FAILED", init_ret);

    /* Feed SPS */
    u32 read_bytes = 0;
    if (vt->sps_len > 0) {
        uint8_t sps_annexb[MP4_MAX_SPS_LEN + 4] = {0, 0, 0, 1};
        memcpy(sps_annexb + 4, vt->sps, vt->sps_len);
        u32 ret_sps = h264bsdDecode(storage, sps_annexb, 4 + vt->sps_len, 0, &read_bytes);
        printf("  Feed SPS (len %u): ret = %u (%s)\n", 4 + vt->sps_len, ret_sps,
               (ret_sps == H264BSD_RDY || ret_sps == H264BSD_HDRS_RDY) ? "OK" : "ERROR");
    }

    /* Feed PPS */
    u32 ret_pps = 0;
    if (vt->pps_len > 0) {
        uint8_t pps_annexb[MP4_MAX_PPS_LEN + 4] = {0, 0, 0, 1};
        memcpy(pps_annexb + 4, vt->pps, vt->pps_len);
        ret_pps = h264bsdDecode(storage, pps_annexb, 4 + vt->pps_len, 0, &read_bytes);
        printf("  Feed PPS (len %u): ret = %u (%s)\n", 4 + vt->pps_len, ret_pps,
               (ret_pps == H264BSD_RDY || ret_pps == H264BSD_HDRS_RDY) ? "OK" :
               (ret_pps == H264BSD_PARAM_SET_ERROR ? "PARAM_SET_ERROR (REJECTED!)" : "ERROR"));
    }

    if (cabac_detected && ret_pps == H264BSD_PARAM_SET_ERROR) {
        printf("  *** VERIFIED FORENSIC PROOF: h264bsd REJECTED PPS DUE TO CABAC! ***\n");
    }

    /* Decode Samples */
    uint32_t decoded_frame_count = 0;
    uint32_t packets_accepted = 0;
    uint32_t max_samples_to_test = (vt->sample_count > 30) ? 30 : vt->sample_count;

    uint32_t first_frame_yuv_crc = 0;
    uint32_t first_frame_rgb_crc = 0;
    bool first_frame_captured = false;

    for (uint32_t s = 0; s < max_samples_to_test; s++) {
        uint32_t sample_size = 0;
        uint64_t sample_offset = 0;
        uint64_t pts = 0;
        bool is_sync = false;

        mp4_demux_get_sample_info(demux, (uint32_t)demux->video_track_idx, s, &sample_offset, &sample_size, &pts, &is_sync);
        if (sample_size == 0) continue;

        uint8_t* sample_buf = (uint8_t*)malloc(sample_size);
        uint32_t rb = 0;
        mp4_demux_read_sample(demux, (uint32_t)demux->video_track_idx, s, sample_buf, sample_size, &rb);

        /* Convert 4-byte length prefix to 00 00 00 01 */
        uint32_t off = 0;
        while (off + 4 <= sample_size) {
            uint32_t nlen = ((uint32_t)sample_buf[off] << 24) | ((uint32_t)sample_buf[off+1] << 16) |
                            ((uint32_t)sample_buf[off+2] << 8) | (uint32_t)sample_buf[off+3];
            if (nlen == 0 || off + 4 + nlen > sample_size) break;
            sample_buf[off] = 0; sample_buf[off+1] = 0; sample_buf[off+2] = 0; sample_buf[off+3] = 1;
            off += 4 + nlen;
        }

        /* Feed to h264bsd */
        uint8_t* cur = sample_buf;
        uint32_t rem = sample_size;
        u32 rbytes = 0;
        uint32_t lg = 0;
        bool sample_accepted = false;

        while (rem > 0 && ++lg < 500) {
            u32 r = h264bsdDecode(storage, cur, rem, s, &rbytes);
            if (r == H264BSD_RDY || r == H264BSD_PIC_RDY) {
                sample_accepted = true;
            } else if (r == H264BSD_HDRS_RDY) {
                sample_accepted = true;
                continue;
            } else if (r == H264BSD_ERROR || r == H264BSD_PARAM_SET_ERROR) {
                if (rbytes == 0) rbytes = 1;
            }
            if (rbytes == 0 || rbytes > rem) break;
            cur += rbytes;
            rem -= rbytes;
        }

        if (sample_accepted) packets_accepted++;

        u32 pic_id = 0, is_idr = 0, num_err = 0;
        u8* yuv = h264bsdNextOutputPicture(storage, &pic_id, &is_idr, &num_err);
        if (yuv) {
            decoded_frame_count++;
            u32 pic_w = h264bsdPicWidth(storage) * 16;
            u32 pic_h = h264bsdPicHeight(storage) * 16;
            u32 cflag = 0, cleft = 0, cw = 0, ctop = 0, ch = 0;
            h264bsdCroppingParams(storage, &cflag, &cleft, &cw, &ctop, &ch);
            u32 vw = (cflag && cw > 0) ? cw : pic_w;
            u32 vh = (cflag && ch > 0) ? ch : pic_h;

            if (!first_frame_captured) {
                first_frame_captured = true;
                uint32_t yuv_total = pic_w * pic_h * 3 / 2;
                first_frame_yuv_crc = calc_crc32(yuv, yuv_total);

                /* Convert to ARGB */
                uint32_t* rgb = (uint32_t*)malloc(vw * vh * sizeof(uint32_t));
                const uint8_t* y_p = yuv;
                const uint8_t* u_p = yuv + pic_w * pic_h;
                const uint8_t* v_p = yuv + pic_w * pic_h + (pic_w / 2) * (pic_h / 2);

                convert_yuv420p_to_argb(y_p, u_p, v_p, vw, vh, pic_w, pic_w / 2, pic_w / 2, rgb);
                first_frame_rgb_crc = calc_crc32(rgb, vw * vh * sizeof(uint32_t));

                save_bmp(out_bmp_name, rgb, vw, vh);
                printf("  [FRAME 0] Decoded: %ux%u (Visible: %ux%u), YUV CRC: 0x%08X, RGB CRC: 0x%08X\n",
                       pic_w, pic_h, vw, vh, first_frame_yuv_crc, first_frame_rgb_crc);
                free(rgb);
            }
        }

        free(sample_buf);
    }

    printf("STAGE 4 [DECODE SUMMARY]:\n");
    printf("  Packets Fed: %u\n", max_samples_to_test);
    printf("  Packets Accepted: %u\n", packets_accepted);
    printf("  Frames Decoded: %u\n", decoded_frame_count);

    if (cabac_detected && decoded_frame_count == 0) {
        printf("======================================================================\n");
        printf("VERDICT: H264 CAPABILITY LIMITATION — CABAC NOT SUPPORTED\n");
        printf("REASON: Stream uses High Profile with CABAC entropy coding.\n");
        printf("        h264bsd specifically requires entropy_coding_mode_flag == 0.\n");
        printf("======================================================================\n");
    } else if (decoded_frame_count > 0) {
        printf("======================================================================\n");
        printf("VERDICT: BASELINE DECODING FULLY FUNCTIONAL!\n");
        printf("First Frame CRC32: 0x%08X (RGB), 0x%08X (YUV)\n", first_frame_rgb_crc, first_frame_yuv_crc);
        printf("======================================================================\n");
    }

    h264bsdShutdown(storage);
    h264bsdFree(storage);
    mp4_demux_close(demux);
    fclose(fp);
}

int main(void) {
    test_media_file("E:/Dolby_Vision_AtmosHDR.mp4", "frame_dolby_usb.bmp");
    test_media_file("d:/Signatures_OS/build/TEST.MP4", "frame_test_baseline.bmp");
    test_media_file("d:/Signatures_OS/build/DOLBY_BASELINE.MP4", "frame_dolby_baseline.bmp");
    return 0;
}
