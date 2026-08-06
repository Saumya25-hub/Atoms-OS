#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t pos;
    uint32_t bit_buf;
    int bits_left;
} RefBits;

static void rbits_init(RefBits* b, const uint8_t* data, size_t sz) {
    b->data = data; b->size = sz; b->pos = 0; b->bit_buf = 0; b->bits_left = 0;
}

static void rbits_refill(RefBits* b) {
    while (b->bits_left <= 24 && b->pos < b->size) {
        uint8_t byte = b->data[b->pos++];
        if (byte == 0xFF) {
            if (b->pos < b->size && b->data[b->pos] == 0x00) {
                b->pos++;
            } else {
                b->pos = b->size;
                break;
            }
        }
        b->bit_buf = (b->bit_buf << 8) | byte;
        b->bits_left += 8;
    }
}

static int rbits_peek(RefBits* b, int n) {
    if (b->bits_left < n) rbits_refill(b);
    return (int)((b->bit_buf >> (b->bits_left - n)) & ((1u << n) - 1));
}

static void rbits_skip(RefBits* b, int n) { b->bits_left -= n; }

static int rbits_get(RefBits* b, int n) {
    int v = rbits_peek(b, n); rbits_skip(b, n); return v;
}

static int ref_jpeg_extend(int v, int t) {
    int vt = 1 << (t - 1);
    return (v < vt) ? (v + (-1 << t) + 1) : v;
}

typedef struct {
    uint8_t bits[17];
    uint8_t huffval[256];
    uint16_t mincode[17];
    int32_t maxcode[18];
    int32_t valptr[17];
    bool valid;
} RefHuff;

static void build_ref_huff(RefHuff* t) {
    int code = 0, p = 0;
    for (int l = 1; l <= 16; l++) {
        int cnt = t->bits[l];
        if (cnt == 0) {
            t->maxcode[l] = -1;
        } else {
            t->valptr[l] = p;
            t->mincode[l] = (uint16_t)code;
            code += cnt - 1;
            t->maxcode[l] = code;
            code += 1;
            p += cnt;
        }
        code <<= 1;
    }
    t->maxcode[17] = -1;
    t->valid = true;
}

static int ref_huff_decode(RefBits* b, const RefHuff* t) {
    if (!t->valid) return -1;
    for (int l = 1; l <= 16; l++) {
        if (b->bits_left < l) rbits_refill(b);
        if (b->bits_left < l) return -1;
        int code = rbits_peek(b, l);
        if (code <= t->maxcode[l]) {
            rbits_skip(b, l);
            int idx = t->valptr[l] + (code - t->mincode[l]);
            return t->huffval[idx];
        }
    }
    return -1;
}

static const uint8_t k_zigzag[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

int main(void) {
    FILE* f = fopen("build/frame30.jpg", "rb");
    if (!f) { printf("Failed to open frame30.jpg\n"); fflush(stdout); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buf = malloc(sz);
    fread(buf, 1, sz, f);
    fclose(f);

    int16_t qtbl[4][64];
    RefHuff huff[8];
    memset(qtbl, 0, sizeof(qtbl));
    memset(huff, 0, sizeof(huff));

    int img_w = 0, img_h = 0, num_comp = 0;
    int comp_h[3] = {1,1,1}, comp_v[3] = {1,1,1}, comp_q[3] = {0,0,0};
    int comp_dc[3] = {0,0,0}, comp_ac[3] = {0,0,0};

    int p = 0;
    int sos_pos = 0;
    while (p < sz - 4) {
        if (buf[p] == 0xFF) {
            uint8_t m = buf[p+1];
            if (m == 0xDB) {
                int len = (buf[p+2] << 8) | buf[p+3];
                int q = p + 4;
                while (q < p + 2 + len) {
                    uint8_t info = buf[q++];
                    uint8_t idx = info & 0xF;
                    uint8_t prec = (info >> 4) & 0xF;
                    for (int i = 0; i < 64; i++) {
                        qtbl[idx][i] = prec ? ((buf[q]<<8)|buf[q+1]) : buf[q];
                        q += prec ? 2 : 1;
                    }
                }
                p += 2 + len;
            } else if (m == 0xC4) {
                int len = (buf[p+2] << 8) | buf[p+3];
                int q = p + 4;
                int seg_end = p + 2 + len;
                while (q < seg_end) {
                    uint8_t info = buf[q++];
                    uint8_t type = (info >> 4) & 1;
                    uint8_t idx = info & 0xF;
                    uint32_t slot = type * 4 + idx;
                    RefHuff* ht = &huff[slot];
                    int tot = 0;
                    for (int i = 1; i <= 16; i++) { ht->bits[i] = buf[q++]; tot += ht->bits[i]; }
                    for (int i = 0; i < tot; i++) ht->huffval[i] = buf[q++];
                    build_ref_huff(ht);
                }
                p += 2 + len;
            } else if (m == 0xC0 || m == 0xC1) {
                int len = (buf[p+2] << 8) | buf[p+3];
                img_h = (buf[p+5] << 8) | buf[p+6];
                img_w = (buf[p+7] << 8) | buf[p+8];
                num_comp = buf[p+9];
                int q = p + 10;
                for (int c = 0; c < num_comp && c < 3; c++) {
                    q++;
                    uint8_t s = buf[q++];
                    comp_h[c] = (s >> 4) & 0xF;
                    comp_v[c] = s & 0xF;
                    comp_q[c] = buf[q++];
                }
                p += 2 + len;
            } else if (m == 0xDA) {
                int len = (buf[p+2] << 8) | buf[p+3];
                int q = p + 4;
                int ns = buf[q++];
                for (int c = 0; c < ns && c < 3; c++) {
                    q++;
                    uint8_t sel = buf[q++];
                    comp_dc[c] = (sel >> 4) & 0xF;
                    comp_ac[c] = 4 + (sel & 0xF);
                }
                sos_pos = p + 2 + len;
                break;
            } else p++;
        } else p++;
    }

    printf("Header Parsed: W=%d H=%d Components=%d SOS_Pos=%d\n", img_w, img_h, num_comp, sos_pos); fflush(stdout);

    RefBits b;
    rbits_init(&b, buf + sos_pos, sz - sos_pos);

    int dc_pred[3] = {0, 0, 0};

    int max_h = comp_h[0], max_v = comp_v[0];
    int mcu_w = max_h * 8;
    int mcu_h = max_v * 8;
    int mcu_cols = (img_w + mcu_w - 1) / mcu_w;
    int mcu_rows = (img_h + mcu_h - 1) / mcu_h;

    int target_bx = 320, target_by = 184;

    int ref_quant[64];
    int ref_dequant[64];
    bool target_found = false;

    for (int my = 0; my < mcu_rows; my++) {
        for (int mx = 0; mx < mcu_cols; mx++) {
            for (int c = 0; c < num_comp; c++) {
                int hs = comp_h[c], vs = comp_v[c];
                const int16_t* qt = qtbl[comp_q[c]];
                if (qt[0] == 0) qt = qtbl[0];

                const RefHuff* dc_ht = &huff[comp_dc[c]];
                if (!dc_ht->valid) dc_ht = &huff[0];
                const RefHuff* ac_ht = &huff[comp_ac[c]];
                if (!ac_ht->valid) ac_ht = &huff[4];

                for (int vy = 0; vy < vs; vy++) {
                    for (int hx = 0; hx < hs; hx++) {
                        int bx = mx * (hs * 8) + hx * 8;
                        int by = my * (vs * 8) + vy * 8;

                        int block[64];
                        int block_q[64];
                        memset(block, 0, sizeof(block));
                        memset(block_q, 0, sizeof(block_q));

                        int dc_sym = ref_huff_decode(&b, dc_ht);
                        int dc_diff = 0;
                        if (dc_sym > 0) {
                            dc_diff = rbits_get(&b, dc_sym);
                            dc_diff = ref_jpeg_extend(dc_diff, dc_sym);
                        }
                        dc_pred[c] += dc_diff;
                        block_q[0] = dc_pred[c];
                        block[0]   = dc_pred[c] * qt[0];

                        int k = 1;
                        while (k < 64) {
                            int ac_sym = ref_huff_decode(&b, ac_ht);
                            if (ac_sym <= 0) break;
                            int run = (ac_sym >> 4) & 0xF;
                            int cat = ac_sym & 0xF;
                            if (run == 15 && cat == 0) { k += 16; continue; }
                            k += run;
                            if (k >= 64) break;
                            int ac_val = rbits_get(&b, cat);
                            ac_val = ref_jpeg_extend(ac_val, cat);
                            block_q[k] = ac_val;
                            block[k]   = ac_val * qt[k];
                            k++;
                        }

                        int raster_q[64];
                        int raster_dq[64];
                        for (int i = 0; i < 64; i++) {
                            raster_q[k_zigzag[i]]  = block_q[i];
                            raster_dq[k_zigzag[i]] = block[i];
                        }

                        if (c == 0 && bx == target_bx && by == target_by) {
                            memcpy(ref_quant, raster_q, sizeof(ref_quant));
                            memcpy(ref_dequant, raster_dq, sizeof(ref_dequant));
                            target_found = true;
                        }
                    }
                }
            }
        }
    }

    if (target_found) {
        printf("\n=== DIRECT BITSTREAM REFERENCE DEQUANTIZER DUMP (BLOCK 320, 184) ===\n");
        printf("REF_DEQUANT_64:\n");
        for (int i = 0; i < 64; i++) printf("%d ", ref_dequant[i]);
        printf("\n\nREF_QUANT_64:\n");
        for (int i = 0; i < 64; i++) printf("%d ", ref_quant[i]);
        printf("\n");
        fflush(stdout);
    } else {
        printf("Target Block (320, 184) not found in stream!\n"); fflush(stdout);
    }

    free(buf);
    return 0;
}
