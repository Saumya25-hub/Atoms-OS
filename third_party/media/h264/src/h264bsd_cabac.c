/*
 * ATOMS OS — H.264 CABAC Decoding Engine Implementation
 * third_party/media/h264/src/h264bsd_cabac.c
 *
 * Derived from ITU-T H.264 (ISO/IEC 14496-10:2005) Section 9.3 and FFmpeg libavcodec
 * License: GNU Lesser General Public License (LGPL) version 2.1 or later
 */

#include "../include/h264bsd_cabac.h"
#include "../include/h264bsd_util.h"
#ifdef ATOMS_USERSPACE
#include <string.h>
#else
#include "kernel/core/lib/include/string.h"
#endif

/*
 * ITU-T H.264 Table 9-44 (rangeTabLPS[64][4])
 */
static const u8 rangeTabLPS[64][4] = {
    {128, 176, 208, 240}, {128, 167, 197, 227}, {128, 158, 187, 216}, {123, 150, 178, 205},
    {116, 142, 169, 195}, {111, 135, 160, 185}, {105, 128, 152, 175}, {100, 122, 144, 166},
    { 95, 116, 137, 158}, { 90, 110, 130, 150}, { 85, 104, 123, 142}, { 81,  99, 117, 135},
    { 77,  94, 111, 128}, { 73,  89, 105, 122}, { 69,  85, 100, 116}, { 66,  80,  95, 110},
    { 62,  76,  90, 104}, { 59,  72,  86,  99}, { 56,  69,  81,  94}, { 53,  65,  77,  89},
    { 51,  62,  73,  85}, { 48,  59,  69,  80}, { 46,  56,  66,  76}, { 43,  53,  63,  72},
    { 41,  50,  59,  69}, { 39,  48,  56,  65}, { 37,  45,  54,  62}, { 35,  43,  51,  59},
    { 33,  41,  48,  56}, { 32,  39,  46,  53}, { 30,  37,  43,  50}, { 29,  35,  41,  48},
    { 27,  33,  39,  45}, { 26,  31,  37,  43}, { 24,  30,  35,  41}, { 23,  28,  33,  39},
    { 22,  27,  32,  37}, { 21,  26,  30,  35}, { 20,  24,  29,  33}, { 19,  23,  27,  31},
    { 18,  22,  26,  30}, { 17,  21,  25,  28}, { 16,  20,  23,  27}, { 15,  19,  22,  25},
    { 14,  17,  21,  24}, { 14,  17,  20,  23}, { 13,  16,  19,  22}, { 12,  15,  18,  21},
    { 12,  14,  17,  20}, { 11,  14,  16,  19}, { 11,  13,  15,  18}, { 10,  12,  15,  17},
    { 10,  12,  14,  16}, {  9,  11,  13,  15}, {  9,  11,  12,  14}, {  8,  10,  12,  14},
    {  8,   9,  11,  13}, {  7,   9,  11,  12}, {  7,   9,  10,  12}, {  7,   8,  10,  11},
    {  6,   8,   9,  11}, {  6,   7,   9,  10}, {  6,   7,   8,   9}, {  2,   2,   2,   2}
};

/*
 * ITU-T H.264 Table 9-45 (transIdxMPS and transIdxLPS)
 */
static const u8 transIdxMPS[64] = {
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63
};

static const u8 transIdxLPS[64] = {
     0,  0,  1,  2,  2,  4,  4,  5,  6,  7,  8,  9,  9, 11, 11, 12,
    13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
    24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
    33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63
};

/* Standard 4x4 zigzag scan order */
static const u8 zigzag_4x4[16] = {
    0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15
};

static const u16 cbf_base[5]   = { 85,  89,  93,  97, 101 };
static const u16 sig_base[5]   = { 105, 120, 134, 149, 152 };
static const u16 last_base[5]  = { 166, 181, 195, 210, 213 };
static const u16 level_base[5] = { 227, 237, 247, 257, 266 };

void h264bsdInitCabacTables(void) {
    /* Tables are statically initialized */
}

static inline u32 cabac_read_bit(CabacDecCtx *c) {
    if (c->bytestream >= c->bytestream_end) return 0;
    u32 bit = (*c->bytestream >> (7 - c->bit_pos)) & 1;
    c->bit_pos++;
    if (c->bit_pos == 8) {
        c->bit_pos = 0;
        c->bytestream++;
    }
    return bit;
}

u32 h264bsdInitCabac(CabacDecCtx *c, strmData_t *pStrmData) {
    if (!c || !pStrmData) return HANTRO_NOK;
    memset(c, 0, sizeof(*c));
    c->pStrmData = pStrmData;

    /* Align to next byte boundary */
    if (!h264bsdIsByteAligned(pStrmData)) {
        u32 bitsToFlush = 8 - (pStrmData->strmBuffReadBits & 7);
        h264bsdFlushBits(pStrmData, bitsToFlush);
    }

    u32 bytesRead = pStrmData->strmBuffReadBits / 8;
    if (bytesRead >= pStrmData->strmBuffSize) {
        return HANTRO_NOK;
    }

    c->bytestream = pStrmData->pStrmBuffStart + bytesRead;
    c->bytestream_end = pStrmData->pStrmBuffStart + pStrmData->strmBuffSize;
    c->bit_pos = 0;

    /* Standard ITU-T Section 9.3.1.2 engine initialization */
    c->range = 510;
    c->low = 0; /* will hold codIOffset */
    for (int i = 0; i < 9; i++) {
        c->low = (c->low << 1) | cabac_read_bit(c);
    }

    return HANTRO_OK;
}

#include "cabac_table_I.h"
#include "cabac_table_PB.h"
#include "h264bsd_slice_header.h"
#include "h264bsd_neighbour.h"

void h264bsdInitCabacContexts(CabacContextTable *tbl, u32 sliceType, i32 sliceQp, u32 cabacInitIdc) {
    if (!tbl) return;

    i32 qp = sliceQp;
    if (qp < 0) qp = 0;
    if (qp > 51) qp = 51;

    const int8_t (*tab)[2];
    if (IS_I_SLICE(sliceType)) {
        tab = cabac_context_init_I;
    } else {
        u32 idc = cabacInitIdc;
        if (idc > 2) idc = 0;
        tab = cabac_context_init_PB[idc];
    }

    for (u32 i = 0; i < 1024 && i < H264_NUM_CABAC_CONTEXTS; i++) {
        i32 m = tab[i][0];
        i32 n = tab[i][1];

        i32 preCtx = ((m * qp) >> 4) + n;
        if (preCtx < 1) preCtx = 1;
        if (preCtx > 126) preCtx = 126;

        u8 valMps;
        u8 pState;
        if (preCtx <= 63) {
            valMps = 0;
            pState = (u8)(63 - preCtx);
        } else {
            valMps = 1;
            pState = (u8)(preCtx - 64);
        }
        tbl->ctx[i] = (pState << 1) | valMps;
    }
}

u32 h264bsdGetCabac(CabacDecCtx *c, u8 *state) {
    u8 s = *state;
    u32 pStateIdx = s >> 1;
    u32 valMPS = s & 1;

    u32 qCodIRangeIdx = (c->range >> 6) & 3;
    u32 codIRangeLPS = rangeTabLPS[pStateIdx][qCodIRangeIdx];
    c->range -= codIRangeLPS;

    u32 binVal;
    if (c->low < c->range) {
        binVal = valMPS;
        pStateIdx = transIdxMPS[pStateIdx];
    } else {
        binVal = 1 - valMPS;
        c->low -= c->range;
        c->range = codIRangeLPS;
        if (pStateIdx == 0) valMPS = 1 - valMPS;
        pStateIdx = transIdxLPS[pStateIdx];
    }
    *state = (pStateIdx << 1) | valMPS;

    while (c->range < 256) {
        c->range <<= 1;
        c->low = (c->low << 1) | cabac_read_bit(c);
    }

    return binVal;
}

u32 h264bsdGetCabacBypass(CabacDecCtx *c) {
    c->low = (c->low << 1) | cabac_read_bit(c);
    u32 binVal;
    if (c->low >= c->range) {
        binVal = 1;
        c->low -= c->range;
    } else {
        binVal = 0;
    }
    return binVal;
}

u32 h264bsdGetCabacTerminate(CabacDecCtx *c) {
    c->range -= 2;
    if (c->low >= c->range) {
        return 1;
    }
    while (c->range < 256) {
        c->range <<= 1;
        c->low = (c->low << 1) | cabac_read_bit(c);
    }
    return 0;
}

static int decode_residual_block(CabacDecCtx *c, CabacContextTable *tbl,
                                 i32 *coeff, u32 cat, u32 maxCoeff, u32 *outCoeffMap) {
    static const int significant_coeff_flag_offset[5] = { 105, 120, 134, 149, 152 };
    static const int last_coeff_flag_offset[5]        = { 166, 181, 195, 210, 213 };
    static const int coeff_abs_level_m1_offset[5]     = { 227, 237, 247, 257, 266 };

    static const uint8_t coeff_abs_level1_ctx[8] = { 1, 2, 3, 4, 0, 0, 0, 0 };
    static const uint8_t coeff_abs_levelgt1_ctx[8] = { 5, 5, 5, 5, 6, 7, 8, 9 };
    static const uint8_t coeff_abs_level_transition[2][8] = {
        { 1, 2, 3, 3, 4, 5, 6, 7 },
        { 4, 4, 4, 4, 5, 6, 7, 7 }
    };

    u32 cbf = h264bsdGetCabac(c, &tbl->ctx[cbf_base[cat]]);
    if (!cbf) return 0;

    int index[64];
    int coeff_count = 0;
    int last;

    uint8_t *sig_ctx_base = &tbl->ctx[significant_coeff_flag_offset[cat]];
    uint8_t *last_ctx_base = &tbl->ctx[last_coeff_flag_offset[cat]];
    uint8_t *abs_level_base = &tbl->ctx[coeff_abs_level_m1_offset[cat]];

    for (last = 0; last < (int)maxCoeff - 1; last++) {
        if (h264bsdGetCabac(c, sig_ctx_base + last)) {
            index[coeff_count++] = last;
            if (h264bsdGetCabac(c, last_ctx_base + last)) {
                last = (int)maxCoeff;
                break;
            }
        }
    }
    if (last == (int)maxCoeff - 1) {
        index[coeff_count++] = last;
    }

    if (coeff_count == 0) return 0;

    int node_ctx = 0;
    u32 coeff_map = 0;
    int total_coeffs = coeff_count;

    while (coeff_count > 0) {
        int idx = index[--coeff_count];
        uint8_t *ctx = abs_level_base + coeff_abs_level1_ctx[node_ctx];
        i32 level = 1;

        if (h264bsdGetCabac(c, ctx) == 0) {
            node_ctx = coeff_abs_level_transition[0][node_ctx];
        } else {
            unsigned coeff_abs = 2;
            ctx = abs_level_base + coeff_abs_levelgt1_ctx[node_ctx];
            node_ctx = coeff_abs_level_transition[1][node_ctx];

            while (coeff_abs < 15 && h264bsdGetCabac(c, ctx)) {
                coeff_abs++;
            }

            if (coeff_abs >= 15) {
                int j = 0;
                while (h264bsdGetCabacBypass(c) && j < 23) {
                    j++;
                }
                coeff_abs = 1;
                while (j-- > 0) {
                    coeff_abs = (coeff_abs << 1) | h264bsdGetCabacBypass(c);
                }
                coeff_abs += 14;
            }
            level = (i32)coeff_abs;
        }

        if (h264bsdGetCabacBypass(c)) {
            level = -level;
        }

        u32 scan_idx = (cat == 0 || cat == 3) ? (u32)idx : zigzag_4x4[idx + (cat == 1 ? 1 : 0)];
        if (scan_idx < 16) {
            coeff[scan_idx] = level;
            coeff_map |= (1 << scan_idx);
        }
    }

    if (outCoeffMap) *outCoeffMap = coeff_map;
    return total_coeffs;
}

static inline i32 decode_cabac_dqp(CabacDecCtx *c, CabacContextTable *tbl) {
    if (h264bsdGetCabac(c, &tbl->ctx[60 + (c->last_qscale_diff != 0 ? 1 : 0)])) {
        int val = 1;
        int ctx = 2;
        while (h264bsdGetCabac(c, &tbl->ctx[60 + ctx])) {
            ctx = 3;
            val++;
            if (val > 102) break;
        }
        i32 dqp = (val & 1) ? ((val + 1) >> 1) : -((val + 1) >> 1);
        c->last_qscale_diff = dqp;
        return dqp;
    } else {
        c->last_qscale_diff = 0;
        return 0;
    }
}

u32 h264bsdDecodeCabacMacroblockLayer(CabacDecCtx *c, CabacContextTable *ctxTbl,
                                       macroblockLayer_t *mbLayer, mbStorage_t *mbStorage,
                                       u32 sliceType, u32 numRefIdxL0Active) {
    if (!c || !ctxTbl || !mbLayer || !mbStorage) return HANTRO_NOK;
    (void)numRefIdxL0Active;

    memset(mbLayer, 0, sizeof(*mbLayer));
    for (u32 b = 0; b < 24; b++) {
        MARK_RESIDUAL_EMPTY(mbLayer->residual.level[b]);
    }

    if (sliceType == 0 || sliceType == 5) { /* P_SLICE */
        /* Check skip_flag */
        u32 skipFlag = h264bsdGetCabac(c, &ctxTbl->ctx[11]);
        if (skipFlag) {
            mbLayer->mbType = P_Skip;
            mbStorage->mbType = P_Skip;
            return HANTRO_OK;
        }

        /* P-slice mb_type */
        u32 mbTypeBin0 = h264bsdGetCabac(c, &ctxTbl->ctx[14]);
        if (mbTypeBin0 == 0) {
            mbLayer->mbType = P_L0_16x16;
            mbStorage->mbType = P_L0_16x16;
        } else {
            u32 mbTypeBin1 = h264bsdGetCabac(c, &ctxTbl->ctx[15]);
            if (mbTypeBin1 == 0) {
                u32 mbTypeBin2 = h264bsdGetCabac(c, &ctxTbl->ctx[16]);
                mbLayer->mbType = mbTypeBin2 ? P_L0_L0_8x16 : P_L0_L0_16x8;
                mbStorage->mbType = mbLayer->mbType;
            } else {
                u32 mbTypeBin2 = h264bsdGetCabac(c, &ctxTbl->ctx[17]);
                if (mbTypeBin2 == 0) {
                    mbLayer->mbType = P_8x8;
                    mbStorage->mbType = P_8x8;
                } else {
                    /* Intra MB in P-slice */
                    mbLayer->mbType = I_4x4;
                    mbStorage->mbType = I_4x4;
                }
            }
        }

        /* Motion vector differences */
        if (mbLayer->mbType <= P_8x8ref0) {
            for (int k = 0; k < 4; k++) {
                mbLayer->mbPred.mvdL0[k].hor = 0;
                mbLayer->mbPred.mvdL0[k].ver = 0;
            }
        }
    } else {
        /* I-slice or IDR-slice */
        int ctx = 0;
        if (mbStorage->mbA && h264bsdIsNeighbourAvailable(mbStorage, mbStorage->mbA)) {
            if (mbStorage->mbA->mbType >= I_16x16_0_0_0 || mbStorage->mbA->mbType == I_PCM)
                ctx++;
        }
        if (mbStorage->mbB && h264bsdIsNeighbourAvailable(mbStorage, mbStorage->mbB)) {
            if (mbStorage->mbB->mbType >= I_16x16_0_0_0 || mbStorage->mbB->mbType == I_PCM)
                ctx++;
        }
        uint8_t *state = (uint8_t*)&ctxTbl->ctx[3];
        u32 bin0 = h264bsdGetCabac(c, &state[ctx]);

        if (bin0 == 0) {
            /* I_4x4 / I_8x8 */
            mbLayer->mbType = I_4x4;
            mbStorage->mbType = I_4x4;

            /* Intra 4x4 prediction modes */
            for (int k = 0; k < 16; k++) {
                u32 prevFlag = h264bsdGetCabac(c, &ctxTbl->ctx[68]);
                mbLayer->mbPred.prevIntra4x4PredModeFlag[k] = prevFlag;
                if (!prevFlag) {
                    u32 b0 = h264bsdGetCabac(c, &ctxTbl->ctx[69]);
                    u32 b1 = h264bsdGetCabac(c, &ctxTbl->ctx[69]);
                    u32 b2 = h264bsdGetCabac(c, &ctxTbl->ctx[69]);
                    mbLayer->mbPred.remIntra4x4PredMode[k] = b0 | (b1 << 1) | (b2 << 2);
                } else {
                    mbLayer->mbPred.remIntra4x4PredMode[k] = 0;
                }
            }

            /* intra_chroma_pred_mode (ctx 64..67) */
            u32 chromaMode = 0;
            if (h264bsdGetCabac(c, &ctxTbl->ctx[64])) {
                chromaMode = 1;
                if (h264bsdGetCabac(c, &ctxTbl->ctx[67])) {
                    chromaMode = 2;
                    if (h264bsdGetCabac(c, &ctxTbl->ctx[67])) chromaMode = 3;
                }
            }
            mbLayer->mbPred.intraChromaPredMode = chromaMode;

            /* Coded Block Pattern (CBP) */
            u32 cbpLuma = 0;
            for (int k = 0; k < 4; k++) {
                cbpLuma |= (h264bsdGetCabac(c, &ctxTbl->ctx[73 + k]) << k);
            }
            u32 cbpChroma = 0;
            if (h264bsdGetCabac(c, &ctxTbl->ctx[77])) {
                cbpChroma = h264bsdGetCabac(c, &ctxTbl->ctx[78]) ? 2 : 1;
            }
            mbLayer->codedBlockPattern = cbpLuma | (cbpChroma << 4);

            /* QP Delta */
            if (mbLayer->codedBlockPattern > 0) {
                mbLayer->mbQpDelta = decode_cabac_dqp(c, ctxTbl);
            } else {
                c->last_qscale_diff = 0;
                mbLayer->mbQpDelta = 0;
            }

            /* Residuals for 4x4 */
            for (int blk = 0; blk < 16; blk++) {
                if (cbpLuma & (1 << (blk / 4))) {
                    i32 coeff[16];
                    memset(coeff, 0, sizeof(coeff));
                    u32 cmap = 0;
                    int numSig = decode_residual_block(c, ctxTbl, coeff, 2, 16, &cmap);
                    if (numSig > 0) {
                        memcpy(mbLayer->residual.level[blk], coeff, 16 * sizeof(i32));
                        mbLayer->residual.totalCoeff[blk] = (i16)numSig;
                        mbLayer->residual.coeffMap[blk] = cmap;
                    }
                }
            }
            if (cbpChroma) {
                i32 coeff[16];
                memset(coeff, 0, sizeof(coeff));
                u32 cmap = 0;
                int sig0 = decode_residual_block(c, ctxTbl, coeff, 3, 4, &cmap);
                if (sig0 > 0) {
                    memcpy(mbLayer->residual.level[25], coeff, 4 * sizeof(i32));
                    mbLayer->residual.totalCoeff[25] = (i16)sig0;
                }
                memset(coeff, 0, sizeof(coeff));
                int sig1 = decode_residual_block(c, ctxTbl, coeff, 3, 4, &cmap);
                if (sig1 > 0) {
                    memcpy(mbLayer->residual.level[25] + 4, coeff, 4 * sizeof(i32));
                    mbLayer->residual.totalCoeff[26] = (i16)sig1;
                }
            }
            if (cbpChroma == 2) {
                for (int blk = 0; blk < 8; blk++) {
                    i32 coeff[16];
                    memset(coeff, 0, sizeof(coeff));
                    u32 cmap = 0;
                    int numSig = decode_residual_block(c, ctxTbl, coeff, 4, 15, &cmap);
                    if (numSig > 0) {
                        memcpy(mbLayer->residual.level[16 + blk], coeff, 16 * sizeof(i32));
                        mbLayer->residual.totalCoeff[16 + blk] = (i16)numSig;
                        mbLayer->residual.coeffMap[16 + blk] = cmap;
                    }
                }
            }
        } else {
            /* I_16x16 or I_PCM */
            state += 2; /* ctx 5 */
            if (h264bsdGetCabacTerminate(c)) {
                mbLayer->mbType = I_PCM;
                mbStorage->mbType = I_PCM;
                return HANTRO_OK;
            }

            u32 cbpLumaFlag = h264bsdGetCabac(c, &state[1]); /* ctx 6 */
            u32 cbpChromaFlag = h264bsdGetCabac(c, &state[2]); /* ctx 7 */
            u32 cbpChroma = 0;
            if (cbpChromaFlag) {
                cbpChroma = h264bsdGetCabac(c, &state[3]) ? 2 : 1; /* ctx 8 */
            }
            u32 modeBit0 = h264bsdGetCabac(c, &state[4]); /* ctx 9 */
            u32 modeBit1 = h264bsdGetCabac(c, &state[5]); /* ctx 10 */
            u32 intra16x16Mode = (modeBit0 << 1) | modeBit1;
            u32 cbpLuma = cbpLumaFlag ? 15 : 0;

            u32 mbTypeVal = 1 + intra16x16Mode + 4 * cbpChroma + 12 * (cbpLumaFlag ? 1 : 0);
            mbLayer->mbType = (mbType_e)(6 + mbTypeVal);
            mbStorage->mbType = mbLayer->mbType;
            mbLayer->codedBlockPattern = cbpLuma | (cbpChroma << 4);

            /* intra_chroma_pred_mode (ctx 64..67) */
            u32 chromaMode = 0;
            if (h264bsdGetCabac(c, &ctxTbl->ctx[64])) {
                chromaMode = 1;
                if (h264bsdGetCabac(c, &ctxTbl->ctx[67])) {
                    chromaMode = 2;
                    if (h264bsdGetCabac(c, &ctxTbl->ctx[67])) chromaMode = 3;
                }
            }
            mbLayer->mbPred.intraChromaPredMode = chromaMode;

            /* mb_qp_delta is ALWAYS decoded for I_16x16 */
            mbLayer->mbQpDelta = decode_cabac_dqp(c, ctxTbl);

            /* Luma DC (block index 24) */
            i32 coeff[16];
            memset(coeff, 0, sizeof(coeff));
            u32 cmap = 0;
            int sigDc = decode_residual_block(c, ctxTbl, coeff, 0, 16, &cmap);
            if (sigDc > 0) {
                memcpy(mbLayer->residual.level[24], coeff, 16 * sizeof(i32));
                mbLayer->residual.totalCoeff[24] = (i16)sigDc;
            }

            /* Luma AC (blocks 0..15) */
            if (cbpLuma) {
                for (int blk = 0; blk < 16; blk++) {
                    memset(coeff, 0, sizeof(coeff));
                    cmap = 0;
                    int numSig = decode_residual_block(c, ctxTbl, coeff, 1, 15, &cmap);
                    if (numSig > 0) {
                        memcpy(mbLayer->residual.level[blk], coeff, 16 * sizeof(i32));
                        mbLayer->residual.totalCoeff[blk] = (i16)numSig;
                        mbLayer->residual.coeffMap[blk] = cmap;
                    }
                }
            }

            /* Chroma DC (blocks 25, 26) */
            if (cbpChroma) {
                memset(coeff, 0, sizeof(coeff));
                cmap = 0;
                int sigChroma0 = decode_residual_block(c, ctxTbl, coeff, 3, 4, &cmap);
                if (sigChroma0 > 0) {
                    memcpy(mbLayer->residual.level[25], coeff, 4 * sizeof(i32));
                    mbLayer->residual.totalCoeff[25] = (i16)sigChroma0;
                }
                memset(coeff, 0, sizeof(coeff));
                int sigChroma1 = decode_residual_block(c, ctxTbl, coeff, 3, 4, &cmap);
                if (sigChroma1 > 0) {
                    memcpy(mbLayer->residual.level[25] + 4, coeff, 4 * sizeof(i32));
                    mbLayer->residual.totalCoeff[26] = (i16)sigChroma1;
                }
            }

            /* Chroma AC (blocks 16..23) */
            if (cbpChroma == 2) {
                for (int blk = 0; blk < 8; blk++) {
                    memset(coeff, 0, sizeof(coeff));
                    cmap = 0;
                    int numSig = decode_residual_block(c, ctxTbl, coeff, 4, 15, &cmap);
                    if (numSig > 0) {
                        memcpy(mbLayer->residual.level[16 + blk], coeff, 16 * sizeof(i32));
                        mbLayer->residual.totalCoeff[16 + blk] = (i16)numSig;
                        mbLayer->residual.coeffMap[16 + blk] = cmap;
                    }
                }
            }
        }
    }

    u32 eos = h264bsdGetCabacTerminate(c);
    if (eos) return 2; /* End of slice */

    return HANTRO_OK;
}
