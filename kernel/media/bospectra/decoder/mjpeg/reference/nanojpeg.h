/*
 * NanoJPEG -- A compact JPEG decoder for C/C++
 * Author: Martin Fiedler <martin.fiedler@gmx.net>
 * Public Domain / MIT Licensed
 */

#ifndef NANOJPEG_H
#define NANOJPEG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _nj_result {
    NJ_OK = 0,
    NJ_NO_JPEG,
    NJ_UNSUPPORTED,
    NJ_OUT_OF_MEM,
    NJ_INTERNAL_ERR,
    NJ_SYNTAX_ERROR,
    __NJ_FINISHED
} nj_result_t;

typedef struct _nj_code {
    uint8_t bits;
    uint8_t code;
} nj_vlc_code_t;

typedef struct _nj_cmp {
    int cid;
    int ssx, ssy;
    int width, height;
    int stride;
    int qtsel;
    int actabsel, dctabsel;
    int dcpred;
    uint8_t *pixels;
} nj_component_t;

typedef struct _nj_ctx {
    nj_result_t error;
    const uint8_t *pos;
    int size;
    int length;
    int width, height;
    int mbwidth, mbheight;
    int mbsizex, mbsizey;
    int ncomp;
    nj_component_t comp[3];
    int qtused, qtavail;
    uint8_t qtab[4][64];
    nj_vlc_code_t vlctab[8][65536];
    int buf, bufbits;
    int block[64];
    int rstinterval;
    uint8_t *rgb;
} nj_context_t;

void njInit(void);
nj_result_t njDecode(const void* jpeg, const int size);
int njGetWidth(void);
int njGetHeight(void);
int njIsColor(void);
uint8_t* njGetImage(void);
int njGetImageSize(void);
uint8_t* njGetComponent(int componentID);
int njGetComponentStride(int componentID);
void njDone(void);

#ifdef NANOJPEG_IMPLEMENTATION

#include "../../../memory/bospectra_memory.h"
#include "kernel/core/lib/include/string.h"

static nj_context_t nj;

static const uint8_t njZZ[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static inline uint8_t njClip(const int x) {
    return (x < 0) ? 0 : ((x > 255) ? 255 : (uint8_t)x);
}

#define NJ_SHOWBITS(n) (nj.buf >> (nj.bufbits - (n)))
#define NJ_FLUSHBITS(n) do { nj.bufbits -= (n); nj.buf &= (1 << nj.bufbits) - 1; } while (0)

static inline void njFillBits(void) {
    while (nj.bufbits < 16) {
        if (nj.size <= 0) {
            nj.buf = (nj.buf << 8) | 0xFF;
            nj.bufbits += 8;
            continue;
        }
        uint8_t b = *nj.pos++;
        nj.size--;
        if (b == 0xFF) {
            if (nj.size > 0 && *nj.pos == 0x00) {
                nj.pos++;
                nj.size--;
            }
        }
        nj.buf = (nj.buf << 8) | b;
        nj.bufbits += 8;
    }
}

static inline int njGetBits(int n) {
    njFillBits();
    int v = NJ_SHOWBITS(n);
    NJ_FLUSHBITS(n);
    return v;
}

static inline int njGetVLC(nj_vlc_code_t* vlc, int* code) {
    njFillBits();
    int val = NJ_SHOWBITS(16);
    int bits = vlc[val].bits;
    if (bits) {
        NJ_FLUSHBITS(bits);
        if (code) *code = vlc[val].code;
        return bits;
    }
    nj.error = NJ_SYNTAX_ERROR;
    return 0;
}

static void njRowIDCT(int* blk) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;

    x0 = (blk[0] << 11) + 128;
    x1 = blk[4] << 11;
    x2 = blk[6];
    x3 = blk[2];
    x4 = blk[1];
    x5 = blk[7];
    x6 = blk[5];
    x7 = blk[3];

    x8 = x0 + x1; x0 -= x1;
    x1 = 2896 * (x3 + x2);
    x2 = x1 + 2156 * x2;
    x3 = x1 - 5040 * x3;
    x1 = x8 + x3; x8 -= x3;
    x3 = x0 + x2; x0 -= x2;
    x2 = 2772 * (x4 + x7);
    x7 = x2 - 5898 * x7;
    x4 = x2 + 896 * x4;
    x2 = 2562 * (x6 + x5);
    x5 = x2 - 7684 * x5;
    x6 = x2 - 2562 * x6;
    x2 = x4 + x6; x4 -= x6;
    x6 = x7 + x5; x7 -= x5;

    blk[0] = (x1 + x6) >> 8;
    blk[7] = (x1 - x6) >> 8;
    blk[1] = (x3 + x2) >> 8;
    blk[6] = (x3 - x2) >> 8;
    blk[2] = (x0 + x4) >> 8;
    blk[5] = (x0 - x4) >> 8;
    blk[3] = (x8 + x7) >> 8;
    blk[4] = (x8 - x7) >> 8;
}

static void njColIDCT(const int* blk, uint8_t* out, int stride) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;

    x0 = (blk[0] << 8) + 8192;
    x1 = blk[32] << 8;
    x2 = blk[48];
    x3 = blk[16];
    x4 = blk[8];
    x5 = blk[56];
    x6 = blk[40];
    x7 = blk[24];

    x8 = x0 + x1; x0 -= x1;
    x1 = 2896 * (x3 + x2);
    x2 = x1 + 2156 * x2;
    x3 = x1 - 5040 * x3;
    x1 = x8 + x3; x8 -= x3;
    x3 = x0 + x2; x0 -= x2;
    x2 = 2772 * (x4 + x7);
    x7 = x2 - 5898 * x7;
    x4 = x2 + 896 * x4;
    x2 = 2562 * (x6 + x5);
    x5 = x2 - 7684 * x5;
    x6 = x2 - 2562 * x6;
    x2 = x4 + x6; x4 -= x6;
    x6 = x7 + x5; x7 -= x5;

    out[0*stride] = njClip((x1 + x6) >> 14);
    out[7*stride] = njClip((x1 - x6) >> 14);
    out[1*stride] = njClip((x3 + x2) >> 14);
    out[6*stride] = njClip((x3 - x2) >> 14);
    out[2*stride] = njClip((x0 + x4) >> 14);
    out[5*stride] = njClip((x0 - x4) >> 14);
    out[3*stride] = njClip((x8 + x7) >> 14);
    out[4*stride] = njClip((x8 - x7) >> 14);
}

static inline void njDecodeBlock(nj_component_t* c, uint8_t* out) {
    int coef, i;
    for (i = 0; i < 64; i++) nj.block[i] = 0;

    njGetVLC(&nj.vlctab[c->dctabsel][0], &coef);
    if (coef) {
        int diff = njGetBits(coef);
        if (diff < (1 << (coef - 1))) diff += (1 << coef) * -1 + 1;
        c->dcpred += diff;
    }
    nj.block[0] = c->dcpred * nj.qtab[c->qtsel][0];

    i = 1;
    while (i < 64) {
        njGetVLC(&nj.vlctab[c->actabsel][0], &coef);
        if (!coef) break;
        if (coef == 0xF0) {
            i += 16;
            continue;
        }
        i += (coef >> 4);
        coef &= 0xF;
        int diff = njGetBits(coef);
        if (diff < (1 << (coef - 1))) diff += (1 << coef) * -1 + 1;
        nj.block[njZZ[i]] = diff * nj.qtab[c->qtsel][i];
        i++;
    }

    for (i = 0; i < 64; i += 8) njRowIDCT(&nj.block[i]);
    for (i = 0; i < 8; i++) njColIDCT(&nj.block[i], &out[i], c->stride);
}

static void njBuildVLC(int idx, const uint8_t* bits, const uint8_t* val) {
    int code = 0, count = 0;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < bits[i]; j++) {
            int code_bits = i + 1;
            int code_val = val[count++];
            int shift = 16 - code_bits;
            for (int k = 0; k < (1 << shift); k++) {
                int index = (code << shift) | k;
                nj.vlctab[idx][index].bits = (uint8_t)code_bits;
                nj.vlctab[idx][index].code = (uint8_t)code_val;
            }
            code++;
        }
        code <<= 1;
    }
}

void njInit(void) {
    for (int i = 0; i < 3; i++) {
        if (nj.comp[i].pixels) {
            bospectra_mem_free(nj.comp[i].pixels);
            nj.comp[i].pixels = NULL;
        }
    }
    if (nj.rgb) {
        bospectra_mem_free(nj.rgb);
        nj.rgb = NULL;
    }
    memset(&nj, 0, sizeof(nj));
}

void njDone(void) {
    njInit();
}

nj_result_t njDecode(const void* jpeg, const int size) {
    njInit();
    nj.pos = (const uint8_t*)jpeg;
    nj.size = size;

    if (nj.size < 2 || nj.pos[0] != 0xFF || nj.pos[1] != 0xD8) return NJ_NO_JPEG;
    nj.pos += 2; nj.size -= 2;

    while (nj.size >= 2 && nj.error == NJ_OK) {
        if (nj.pos[0] != 0xFF) { nj.pos++; nj.size--; continue; }
        while (nj.size > 0 && nj.pos[0] == 0xFF) { nj.pos++; nj.size--; }
        if (nj.size == 0) break;
        uint8_t m = nj.pos[0];
        nj.pos++; nj.size--;
        if (m == 0xD9) break; /* EOI */

        int len = (nj.pos[0] << 8) | nj.pos[1];
        const uint8_t* p = nj.pos + 2;
        int remaining = len - 2;

        if (m == 0xC0 || m == 0xC1) { /* SOF0 / SOF1 */
            nj.height = (p[1] << 8) | p[2];
            nj.width  = (p[3] << 8) | p[4];
            nj.ncomp  = p[5];
            if (nj.ncomp > 3) return NJ_UNSUPPORTED;
            int max_ssx = 1, max_ssy = 1;
            const uint8_t* cp = p + 6;
            for (int i = 0; i < nj.ncomp; i++) {
                nj.comp[i].cid = cp[0];
                nj.comp[i].ssx = (cp[1] >> 4) & 0xF;
                nj.comp[i].ssy = cp[1] & 0xF;
                nj.comp[i].qtsel = cp[2];
                if (nj.comp[i].ssx > max_ssx) max_ssx = nj.comp[i].ssx;
                if (nj.comp[i].ssy > max_ssy) max_ssy = nj.comp[i].ssy;
                cp += 3;
            }
            nj.mbsizex = max_ssx * 8;
            nj.mbsizey = max_ssy * 8;
            nj.mbwidth  = (nj.width  + nj.mbsizex - 1) / nj.mbsizex;
            nj.mbheight = (nj.height + nj.mbsizey - 1) / nj.mbsizey;
            for (int i = 0; i < nj.ncomp; i++) {
                nj.comp[i].width  = (nj.width * nj.comp[i].ssx + max_ssx - 1) / max_ssx;
                nj.comp[i].height = (nj.height * nj.comp[i].ssy + max_ssy - 1) / max_ssy;
                nj.comp[i].stride = nj.mbwidth * nj.comp[i].ssx * 8;
                int buf_size = nj.comp[i].stride * nj.mbheight * nj.comp[i].ssy * 8;
                nj.comp[i].pixels = (uint8_t*)bospectra_mem_alloc(buf_size, "NanoJPEGPlane");
                if (!nj.comp[i].pixels) return NJ_OUT_OF_MEM;
                memset(nj.comp[i].pixels, 0, buf_size);
            }
        } else if (m == 0xDB) { /* DQT */
            const uint8_t* qp = p;
            while (remaining >= 65) {
                int qt = qp[0] & 0xF;
                qp++; remaining--;
                for (int i = 0; i < 64; i++) {
                    nj.qtab[qt][i] = qp[i];
                }
                qp += 64; remaining -= 64;
            }
        } else if (m == 0xC4) { /* DHT */
            const uint8_t* hp = p;
            while (remaining > 17) {
                int ht = hp[0];
                int idx = ((ht >> 4) & 1) * 4 + (ht & 0xF);
                hp++; remaining--;
                uint8_t bits[16];
                int num_syms = 0;
                for (int i = 0; i < 16; i++) {
                    bits[i] = hp[i];
                    num_syms += bits[i];
                }
                hp += 16; remaining -= 16;
                if (remaining < num_syms) break;
                njBuildVLC(idx, bits, hp);
                hp += num_syms; remaining -= num_syms;
            }
        } else if (m == 0xDD) { /* DRI */
            nj.rstinterval = (p[0] << 8) | p[1];
        } else if (m == 0xDA) { /* SOS */
            int ns = p[0];
            const uint8_t* sp = p + 1;
            for (int i = 0; i < ns; i++) {
                int cid = sp[0];
                int htsel = sp[1];
                for (int c = 0; c < nj.ncomp; c++) {
                    if (nj.comp[c].cid == cid) {
                        nj.comp[c].dctabsel = (htsel >> 4) & 0xF;
                        nj.comp[c].actabsel = 4 + (htsel & 0xF);
                        break;
                    }
                }
                sp += 2;
            }
            nj.pos += len; nj.size -= len;
            /* Decode Scan Data */
            nj.buf = 0; nj.bufbits = 0;
            for (int c = 0; c < nj.ncomp; c++) nj.comp[c].dcpred = 0;
            int rst_count = 0;
            for (int mby = 0; mby < nj.mbheight; mby++) {
                for (int mbx = 0; mbx < nj.mbwidth; mbx++) {
                    if (nj.rstinterval && rst_count == nj.rstinterval) {
                        nj.buf = 0; nj.bufbits = 0;
                        while (nj.size > 0 && nj.pos[0] != 0xFF) { nj.pos++; nj.size--; }
                        if (nj.size >= 2 && nj.pos[0] == 0xFF) { nj.pos += 2; nj.size -= 2; }
                        for (int c = 0; c < nj.ncomp; c++) nj.comp[c].dcpred = 0;
                        rst_count = 0;
                    }
                    rst_count++;
                    for (int c = 0; c < nj.ncomp; c++) {
                        for (int sy = 0; sy < nj.comp[c].ssy; sy++) {
                            for (int sx = 0; sx < nj.comp[c].ssx; sx++) {
                                uint8_t* out = nj.comp[c].pixels +
                                    (mby * nj.comp[c].ssy + sy) * 8 * nj.comp[c].stride +
                                    (mbx * nj.comp[c].ssx + sx) * 8;
                                njDecodeBlock(&nj.comp[c], out);
                                if (nj.error != NJ_OK) return nj.error;
                            }
                        }
                    }
                }
            }
            break; /* Scan completed */
        } else {
            nj.pos += len; nj.size -= len;
        }
    }
    return NJ_OK;
}

int njGetWidth(void)  { return nj.width; }
int njGetHeight(void) { return nj.height; }
int njIsColor(void)   { return nj.ncomp > 1; }

uint8_t* njGetComponent(int componentID) {
    if (componentID < 0 || componentID >= 3) return NULL;
    return nj.comp[componentID].pixels;
}

int njGetComponentStride(int componentID) {
    if (componentID < 0 || componentID >= 3) return 0;
    return nj.comp[componentID].stride;
}

#endif /* NANOJPEG_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* NANOJPEG_H */
