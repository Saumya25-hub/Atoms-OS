/*
 * ATOMS OS — H.264 CABAC Decoding Engine Header
 * third_party/media/h264/include/h264bsd_cabac.h
 *
 * Derived from ITU-T H.264 (ISO/IEC 14496-10) and FFmpeg libavcodec
 * License: GNU Lesser General Public License (LGPL) version 2.1 or later
 */

#ifndef H264BSD_CABAC_H
#define H264BSD_CABAC_H

#include "basetype.h"
#include "h264bsd_stream.h"
#include "h264bsd_macroblock_layer.h"

#define H264_NUM_CABAC_CONTEXTS 1024

typedef struct {
    u32 low;
    u32 range;
    const u8 *bytestream;
    const u8 *bytestream_end;
    strmData_t *pStrmData;
    u32 bit_pos;
    i32 last_qscale_diff;
} CabacDecCtx;

/* Context state array: 6-bit pStateIdx and 1-bit valMPS in each byte */
typedef struct {
    u8 ctx[H264_NUM_CABAC_CONTEXTS];
} CabacContextTable;

/* Function prototypes */
void h264bsdInitCabacTables(void);
u32  h264bsdInitCabac(CabacDecCtx *c, strmData_t *pStrmData);
void h264bsdInitCabacContexts(CabacContextTable *tbl, u32 sliceType, i32 sliceQp, u32 cabacInitIdc);

u32  h264bsdGetCabac(CabacDecCtx *c, u8 *state);
u32  h264bsdGetCabacBypass(CabacDecCtx *c);
u32  h264bsdGetCabacTerminate(CabacDecCtx *c);

/* Macroblock CABAC parsing */
u32  h264bsdDecodeCabacMacroblockLayer(CabacDecCtx *c, CabacContextTable *ctxTbl,
                                       macroblockLayer_t *mbLayer, mbStorage_t *mbStorage,
                                       u32 sliceType, u32 numRefIdxL0Active);

#endif /* H264BSD_CABAC_H */
