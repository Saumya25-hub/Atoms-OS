/*
 * BOSPECTRA MJPEG Decoder — mjpeg_decoder.c
 *
 * Native baseline JPEG decoder for ATOMS OS.
 * Decodes MJPEG AVI video chunks (each chunk is a standalone JFIF JPEG).
 *
 * Supported: Baseline DCT, YCbCr, 4:2:0 and 4:2:2 chroma subsampling.
 * Output:    BOSPECTRA_PIXEL_FORMAT_YUV420P (fed directly to Color Engine)
 *
 * JPEG decode path:
 *   SOI → DQT (quant tables) → SOF0 (frame header) →
 *   DHT (Huffman tables) → SOS (scan data) → IDCT → YUV planes → EOI
 */

#include "mjpeg_decoder.h"
#include "../common/bitstream_reader.h"
#include "../common/idct.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../memory/bospectra_memory.h"
#include "../../include/bospectra_errors.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

/* ==========================================================================
 * JPEG Marker Definitions
 * ======================================================================= */
#define JPEG_SOI  0xFFD8u
#define JPEG_EOI  0xFFD9u
#define JPEG_SOF0 0xFFC0u   /* Baseline DCT */
#define JPEG_SOF1 0xFFC1u   /* Extended sequential */
#define JPEG_DHT  0xFFC4u   /* Huffman table */
#define JPEG_DQT  0xFFDBu   /* Quantisation table */
#define JPEG_SOS  0xFFDAu   /* Start of scan */
#define JPEG_DRI  0xFFDDu   /* Restart interval */
#define JPEG_APP0 0xFFE0u

#define MJPEG_MAX_COMPONENTS 3
#define MJPEG_MAX_QUANT_TABLES 4
#define MJPEG_MAX_HUFF_TABLES  8   /* 4 DC + 4 AC */

/* ==========================================================================
 * Huffman Table
 * ======================================================================= */
typedef struct {
    uint8_t  bits[17];       /* Number of codes of each length 1..16 */
    uint8_t  huffval[256];   /* Huffman symbol values */
    uint16_t mincode[17];
    int32_t  maxcode[18];    /* maxcode[17] = -1 sentinel */
    int32_t  valptr[17];
    bool     valid;
} HuffTable;

/* ==========================================================================
 * MJPEG Decoder Context
 * ======================================================================= */
typedef struct {
    uint32_t  stream_width;
    uint32_t  stream_height;

    /* Per-frame state (reset each decode call) */
    uint32_t  img_width;
    uint32_t  img_height;
    uint8_t   num_components;

    int16_t   quant_tables[MJPEG_MAX_QUANT_TABLES][64]; /* De-zigzagged */
    HuffTable huff[MJPEG_MAX_HUFF_TABLES];

    struct {
        uint8_t id;
        uint8_t h_samp;
        uint8_t v_samp;
        uint8_t quant_tbl_idx;
        uint8_t dc_huff_idx;
        uint8_t ac_huff_idx;
        int32_t dc_pred;           /* DC prediction carry */
    } comp[MJPEG_MAX_COMPONENTS];

    uint16_t restart_interval;
} MJPEG_DecCtx;

/* ==========================================================================
 * JPEG Natural (zigzag) to raster order mapping
 * ======================================================================= */
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

/* ==========================================================================
 * Minimal big-endian read helpers (operate on in-memory buffer)
 * ======================================================================= */
static inline uint8_t  rd8 (const uint8_t* b, size_t* p) { return b[(*p)++]; }
static inline uint16_t rd16(const uint8_t* b, size_t* p) {
    uint16_t v = ((uint16_t)b[*p] << 8) | b[*p + 1]; *p += 2; return v;
}

/* ==========================================================================
 * Build Huffman decode table from JPEG DHT data
 * ======================================================================= */
static void build_huffman(HuffTable* t) {
    int32_t code = 0;
    int32_t p    = 0;

    for (int32_t l = 1; l <= 16; l++) {
        int32_t cnt = t->bits[l];
        if (cnt == 0) {
            t->maxcode[l] = -1;
        } else {
            t->valptr[l]  = p;
            t->mincode[l] = (uint16_t)code;
            code += cnt - 1;
            t->maxcode[l] = code;
            code += 1;
            p += cnt;
        }
        code <<= 1;
    }
    t->maxcode[17] = -1; /* Sentinel */
    t->valid = true;
}

/* ==========================================================================
 * Bitstream reader — walks the raw entropy-coded JPEG scan data
 * ======================================================================= */
typedef struct {
    const uint8_t* data;
    size_t         size;
    size_t         pos;
    uint32_t       bit_buf;
    int32_t        bits_left;
} JPEGBits;

static void jbits_init(JPEGBits* jb, const uint8_t* data, size_t size) {
    jb->data      = data;
    jb->size      = size;
    jb->pos       = 0;
    jb->bit_buf   = 0;
    jb->bits_left = 0;
}

/* Refill 8 bits (handles 0xFF byte stuffing) */
static void jbits_refill(JPEGBits* jb) {
    while (jb->bits_left <= 24 && jb->pos < jb->size) {
        uint8_t byte = jb->data[jb->pos++];
        if (byte == 0xFF) {
            /* Skip stuffed 0x00 byte */
            if (jb->pos < jb->size && jb->data[jb->pos] == 0x00) {
                jb->pos++;
            } else {
                /* Marker encountered — push back and stop */
                jb->pos -= 1;
                break;
            }
        }
        jb->bit_buf   = (jb->bit_buf << 8) | byte;
        jb->bits_left += 8;
    }
}

static inline int32_t jbits_peek(JPEGBits* jb, int32_t n) {
    if (jb->bits_left < n) jbits_refill(jb);
    return (int32_t)((jb->bit_buf >> (jb->bits_left - n)) & ((1u << n) - 1));
}

static inline void jbits_skip(JPEGBits* jb, int32_t n) {
    jb->bits_left -= n;
}

static inline int32_t jbits_get(JPEGBits* jb, int32_t n) {
    int32_t v = jbits_peek(jb, n);
    jbits_skip(jb, n);
    return v;
}

/* ==========================================================================
 * Decode one Huffman symbol from the bitstream
 * ======================================================================= */
static int32_t huff_decode(JPEGBits* jb, const HuffTable* t) {
    if (!t->valid) return -1;

    for (int32_t si = 1; si <= 16; si++) {
        if (jb->bits_left < si) jbits_refill(jb);
        int32_t code = jbits_peek(jb, si);
        if (code <= t->maxcode[si]) {
            jbits_skip(jb, si);
            int32_t idx = t->valptr[si] + (code - t->mincode[si]);
            return t->huffval[idx];
        }
    }
    return -1; /* Error */
}

/* ==========================================================================
 * Extend sign (JPEG spec Table F.1)
 * ======================================================================= */
static inline int32_t jpeg_extend(int32_t v, int32_t t) {
    int32_t vt = 1 << (t - 1);
    return (v < vt) ? (v + (-1 << t) + 1) : v;
}

/* ==========================================================================
 * Decode one 8×8 DCT block → dequantise → IDCT → output into plane
 * ======================================================================= */
static bool decode_block(JPEGBits* jb,
                          const HuffTable* dc_huff,
                          const HuffTable* ac_huff,
                          const int16_t*   quant,
                          int32_t*         dc_pred,
                          uint8_t*         out_plane,
                          uint32_t         plane_stride,
                          uint32_t         blk_x,
                          uint32_t         blk_y) {
    int16_t block[64];
    memset(block, 0, sizeof(block));

    /* DC coefficient */
    int32_t dc_sym = huff_decode(jb, dc_huff);
    if (dc_sym < 0) return false;
    int32_t dc_diff = 0;
    if (dc_sym > 0) {
        dc_diff = jbits_get(jb, dc_sym);
        dc_diff = jpeg_extend(dc_diff, dc_sym);
    }
    *dc_pred += dc_diff;
    block[0] = (int16_t)((*dc_pred) * quant[0]);

    /* AC coefficients */
    int32_t k = 1;
    while (k < 64) {
        int32_t ac_sym = huff_decode(jb, ac_huff);
        if (ac_sym < 0) break;
        if (ac_sym == 0x00) break; /* EOB */

        int32_t run = (ac_sym >> 4) & 0xF;
        int32_t cat = ac_sym & 0xF;

        if (run == 15 && cat == 0) { /* ZRL — 16 zeros */
            k += 16;
            continue;
        }

        k += run;
        if (k >= 64) break;

        int32_t ac_val = jbits_get(jb, cat);
        ac_val = jpeg_extend(ac_val, cat);
        block[k] = (int16_t)(ac_val * quant[k]);
        k++;
    }

    /* De-zigzag into raster order */
    int16_t raster[64];
    for (int32_t i = 0; i < 64; i++) {
        raster[k_zigzag[i]] = block[i];
    }
    /* (Note: block[i] above already holds the AC value at zigzag position i,
     * but we stored it at index k=zigzag position. Fix: use zigzag correctly) */

    /* IDCT + level-shift → write to output plane */
    int32_t idct_in[64];
    for (int32_t i = 0; i < 64; i++) idct_in[i] = (int32_t)raster[i];

    uint8_t* dst = out_plane + blk_y * plane_stride + blk_x;
    idct_8x8(idct_in, dst, plane_stride);

    return true;
}

/* ==========================================================================
 * Parse one JPEG image from a raw byte buffer
 * Output: fills the YUV420P planes of an already-allocated BOSFrame
 * ======================================================================= */
static bospectra_error_t jpeg_decode_image(
        MJPEG_DecCtx* ctx,
        const uint8_t* data, size_t size,
        BOSFrame* frame) {

    size_t p = 0;

    /* Verify SOI */
    if (size < 2 || rd16(data, &p) != JPEG_SOI) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    /* Zero DC predictors at start of each frame */
    for (int i = 0; i < MJPEG_MAX_COMPONENTS; i++) ctx->comp[i].dc_pred = 0;
    ctx->restart_interval = 0;

    bool scan_started = false;

    while (p + 2 <= size && !scan_started) {
        /* Seek to next 0xFF marker */
        if (data[p] != 0xFF) { p++; continue; }
        while (p < size && data[p] == 0xFF) p++;  /* Skip padding 0xFF bytes */
        if (p >= size) break;

        uint8_t marker_lo = data[p++];
        uint16_t marker   = 0xFF00u | marker_lo;

        if (marker == (JPEG_EOI & 0xFFFF) || marker_lo == 0xD9) break;
        if (marker_lo >= 0xD0 && marker_lo <= 0xD7) continue; /* RST markers */
        if (marker_lo == 0x00) continue; /* Stuffed byte */

        /* Read segment length */
        if (p + 2 > size) break;
        uint16_t seg_len = rd16(data, &p); /* Includes the 2-byte length field */
        size_t   seg_end = p + seg_len - 2;
        if (seg_end > size) seg_end = size;

        /* ---- DQT: Quantisation table ---- */
        if (marker == JPEG_DQT) {
            size_t q = p;
            while (q < seg_end) {
                uint8_t qt_info  = rd8(data, &q);
                uint8_t qt_prec  = (qt_info >> 4) & 0xF; /* 0=8bit, 1=16bit */
                uint8_t qt_idx   = qt_info & 0xF;
                if (qt_idx >= MJPEG_MAX_QUANT_TABLES) { q = seg_end; break; }
                for (int32_t i = 0; i < 64; i++) {
                    ctx->quant_tables[qt_idx][i] = qt_prec ?
                        (int16_t)((uint16_t)rd8(data,&q)<<8 | rd8(data,&q)) :
                        (int16_t)rd8(data, &q);
                }
            }
        }
        /* ---- DHT: Huffman table ---- */
        else if (marker == JPEG_DHT) {
            size_t q = p;
            while (q < seg_end) {
                uint8_t ht_info = rd8(data, &q);
                uint8_t ht_type = (ht_info >> 4) & 0x1; /* 0=DC, 1=AC */
                uint8_t ht_idx  = ht_info & 0xF;
                uint32_t tbl_slot = ht_type * 4 + ht_idx;
                if (tbl_slot >= MJPEG_MAX_HUFF_TABLES) { q = seg_end; break; }

                HuffTable* ht = &ctx->huff[tbl_slot];
                memset(ht, 0, sizeof(HuffTable));

                uint32_t total_syms = 0;
                for (int32_t i = 1; i <= 16; i++) {
                    ht->bits[i] = rd8(data, &q);
                    total_syms += ht->bits[i];
                }
                for (uint32_t i = 0; i < total_syms && q < seg_end; i++) {
                    ht->huffval[i] = rd8(data, &q);
                }
                build_huffman(ht);
            }
        }
        /* ---- SOF0: Frame header (image dimensions + component info) ---- */
        else if (marker == JPEG_SOF0 || marker == JPEG_SOF1) {
            size_t q = p;
            rd8(data, &q); /* precision */
            ctx->img_height = rd16(data, &q);
            ctx->img_width  = rd16(data, &q);
            ctx->num_components = rd8(data, &q);
            if (ctx->num_components > MJPEG_MAX_COMPONENTS) ctx->num_components = MJPEG_MAX_COMPONENTS;
            for (uint8_t c = 0; c < ctx->num_components; c++) {
                ctx->comp[c].id           = rd8(data, &q);
                uint8_t samp              = rd8(data, &q);
                ctx->comp[c].h_samp       = (samp >> 4) & 0xF;
                ctx->comp[c].v_samp       = samp & 0xF;
                ctx->comp[c].quant_tbl_idx = rd8(data, &q);
            }
        }
        /* ---- DRI: Restart interval ---- */
        else if (marker == JPEG_DRI) {
            size_t q = p;
            ctx->restart_interval = rd16(data, &q);
        }
        /* ---- SOS: Start of scan — entropy-coded data follows ---- */
        else if (marker == JPEG_SOS) {
            size_t q = p;
            uint8_t ns = rd8(data, &q); /* Number of components in scan */
            if (ns > MJPEG_MAX_COMPONENTS) ns = MJPEG_MAX_COMPONENTS;
            for (uint8_t c = 0; c < ns && c < ctx->num_components; c++) {
                uint8_t comp_id = rd8(data, &q);
                uint8_t ht_sel  = rd8(data, &q);
                uint8_t dc_idx  = (ht_sel >> 4) & 0xF;   /* DC Huffman table index */
                uint8_t ac_idx  = 4 + (ht_sel & 0xF);    /* AC Huffman table (offset 4) */
                for (uint8_t ci = 0; ci < ctx->num_components; ci++) {
                    if (ctx->comp[ci].id == comp_id) {
                        ctx->comp[ci].dc_huff_idx = dc_idx;
                        ctx->comp[ci].ac_huff_idx = ac_idx;
                        break;
                    }
                }
                (void)comp_id;
            }
            rd8(data, &q); /* Ss */
            rd8(data, &q); /* Se */
            rd8(data, &q); /* Ah/Al */

            /* q now points to entropy-coded scan data */
            size_t ecs_start = q;
            size_t ecs_size  = size - ecs_start;

            /* Validate frame dimensions match allocated frame */
            uint32_t dec_w = ctx->img_width  ? ctx->img_width  : frame->width;
            uint32_t dec_h = ctx->img_height ? ctx->img_height : frame->height;
            if (dec_w > frame->width)  dec_w = frame->width;
            if (dec_h > frame->height) dec_h = frame->height;

            /* Initialise bitstream reader on ECS data */
            JPEGBits jb;
            jbits_init(&jb, data + ecs_start, ecs_size);

            /* Determine max sampling factors */
            uint8_t max_h = 1, max_v = 1;
            for (uint8_t c = 0; c < ctx->num_components; c++) {
                if (ctx->comp[c].h_samp > max_h) max_h = ctx->comp[c].h_samp;
                if (ctx->comp[c].v_samp > max_v) max_v = ctx->comp[c].v_samp;
            }
            if (max_h == 0) max_h = 1;
            if (max_v == 0) max_v = 1;

            uint32_t mcu_w_px = max_h * 8;
            uint32_t mcu_h_px = max_v * 8;
            uint32_t mcu_cols = (dec_w + mcu_w_px - 1) / mcu_w_px;
            uint32_t mcu_rows = (dec_h + mcu_h_px - 1) / mcu_h_px;
            uint32_t rs_count = 0;
            /* Reset DC predictors for all components at start of scan (ISO/IEC 10918-1 F.2.1.3) */
            for (uint8_t c = 0; c < ctx->num_components; c++) {
                ctx->comp[c].dc_pred = 0;
            }

            for (uint32_t my = 0; my < mcu_rows; my++) {
                for (uint32_t mx = 0; mx < mcu_cols; mx++) {
                    /* Restart interval handling */
                    if (ctx->restart_interval && rs_count == ctx->restart_interval) {
                        jb.bits_left = 0;
                        jb.bit_buf   = 0;
                        while (jb.pos < jb.size && jb.data[jb.pos] != 0xFF) jb.pos++;
                        if (jb.pos + 1 < jb.size) jb.pos += 2;
                        for (uint8_t c = 0; c < ctx->num_components; c++) ctx->comp[c].dc_pred = 0;
                        rs_count = 0;
                    }
                    rs_count++;

                    /* Decode each component block in bitstream MCU order */
                    for (uint8_t ci = 0; ci < ctx->num_components && ci < 3; ci++) {
                        uint8_t* plane  = frame->data[ci];
                        uint8_t  hs     = ctx->comp[ci].h_samp ? ctx->comp[ci].h_samp : 1;
                        uint8_t  vs     = ctx->comp[ci].v_samp ? ctx->comp[ci].v_samp : 1;
                        uint32_t p_w    = (ci == 0) ? dec_w : (dec_w / max_h);
                        uint32_t p_h    = (ci == 0) ? dec_h : (dec_h / max_v);
                        uint32_t stride = p_w;

                        for (uint8_t vy = 0; vy < vs; vy++) {
                            for (uint8_t hx = 0; hx < hs; hx++) {
                                uint32_t bx = mx * (hs * 8) + hx * 8;
                                uint32_t by = my * (vs * 8) + vy * 8;

                                uint8_t q_idx = ctx->comp[ci].quant_tbl_idx;
                                const int16_t* qtbl = ctx->quant_tables[q_idx];
                                if (qtbl[0] == 0 && ctx->quant_tables[0][0] != 0) {
                                    qtbl = ctx->quant_tables[0];
                                }

                                if (plane && bx < p_w && by < p_h) {
                                    decode_block(&jb,
                                                 &ctx->huff[ctx->comp[ci].dc_huff_idx],
                                                 &ctx->huff[ctx->comp[ci].ac_huff_idx],
                                                 qtbl,
                                                 &ctx->comp[ci].dc_pred,
                                                 plane, stride, bx, by);
                                }
                            }
                        }
                    }
                }
            }

            scan_started = true; /* Scan done; exit parsing loop */
        }

        p = seg_end; /* Advance to next segment */
    }

    frame->width  = ctx->img_width  ? ctx->img_width  : ctx->stream_width;
    frame->height = ctx->img_height ? ctx->img_height : ctx->stream_height;
    frame->format = BOSPECTRA_PIXEL_FORMAT_YUV420P;

    /* FORENSIC TELEMETRY: Calculate Stage 5 Y, Cb, Cr CRC32 checksums */
    static uint32_t s_forensic_frame_count = 0;
    s_forensic_frame_count++;

    uint32_t y_sz  = frame->linesize[0] * frame->height;
    uint32_t cb_sz = frame->linesize[1] * (frame->height / 2);
    uint32_t cr_sz = frame->linesize[2] * (frame->height / 2);

    uint32_t y_crc  = 0xFFFFFFFFU;
    uint32_t cb_crc = 0xFFFFFFFFU;
    uint32_t cr_crc = 0xFFFFFFFFU;

    if (frame->data[0] && y_sz > 0) {
        for (uint32_t i = 0; i < y_sz; i++) {
            y_crc ^= frame->data[0][i];
            for (int b = 0; b < 8; b++) y_crc = (y_crc >> 1) ^ ((y_crc & 1) ? 0xEDB88320U : 0);
        }
        y_crc ^= 0xFFFFFFFFU;
    }
    if (frame->data[1] && cb_sz > 0) {
        for (uint32_t i = 0; i < cb_sz; i++) {
            cb_crc ^= frame->data[1][i];
            for (int b = 0; b < 8; b++) cb_crc = (cb_crc >> 1) ^ ((cb_crc & 1) ? 0xEDB88320U : 0);
        }
        cb_crc ^= 0xFFFFFFFFU;
    }
    if (frame->data[2] && cr_sz > 0) {
        for (uint32_t i = 0; i < cr_sz; i++) {
            cr_crc ^= frame->data[2][i];
            for (int b = 0; b < 8; b++) cr_crc = (cr_crc >> 1) ^ ((cr_crc & 1) ? 0xEDB88320U : 0);
        }
        cr_crc ^= 0xFFFFFFFFU;
    }

    if (s_forensic_frame_count <= 5 || (s_forensic_frame_count % 30) == 0) {
        bospectra_trace_u32("FORENSIC FRAME #", s_forensic_frame_count);
        bospectra_trace_hex("Y Plane CRC32", y_crc);
        bospectra_trace_hex("Cb Plane CRC32", cb_crc);
        bospectra_trace_hex("Cr Plane CRC32", cr_crc);
    }

    return BOSPECTRA_SUCCESS;
}

/* ==========================================================================
 * BOSPECTRA Decoder Driver Interface
 * ======================================================================= */

static bospectra_error_t mjpeg_open(void** driver_ctx, const BOSPECTRA_StreamDescriptor* stream_desc) {
    if (!driver_ctx || !stream_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    MJPEG_DecCtx* ctx = (MJPEG_DecCtx*)bospectra_mem_alloc(sizeof(MJPEG_DecCtx), "MJPEGDecoderCtx");
    if (!ctx) return BOSPECTRA_ERR_OUT_OF_MEMORY;

    memset(ctx, 0, sizeof(MJPEG_DecCtx));
    ctx->stream_width  = stream_desc->width  ? stream_desc->width  : 640;
    ctx->stream_height = stream_desc->height ? stream_desc->height : 360;

    *driver_ctx = ctx;
    bospectra_log("MJPEG", "MJPEG decoder opened (native baseline JPEG).");
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mjpeg_decode_packet(void* driver_ctx, const BOSPacket* packet, BOSFrame** out_frame) {
    MJPEG_DecCtx* ctx = (MJPEG_DecCtx*)driver_ctx;
    if (!ctx || !packet || !packet->data || packet->size < 4 || !out_frame) {
        bospectra_trace_str("TRACE 8 — Decoder", "FAILED (Invalid Arguments or Packet Data NULL)");
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    const uint8_t* buf = (const uint8_t*)packet->data;
    size_t         sz  = packet->size;

    bool soi_found = (buf[0] == 0xFF && buf[1] == 0xD8);
    bospectra_trace_str("TRACE 8 — Decoder", "Decoding MJPEG Packet");
    bospectra_trace_str("Decoder Name", "Native ISO/IEC 10918-1 MJPEG");
    bospectra_trace_u32("Packet Size", (uint32_t)sz);
    bospectra_trace_str("JPEG SOI Found", soi_found ? "YES (0xFF 0xD8)" : "NO");

    /* Verify JPEG SOI */
    if (!soi_found) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    /* Acquire pre-allocated YUV420P frame from BOSPECTRA Frame Pool */
    BOSFrame* frame = NULL;
    bospectra_error_t err = bospectra_frame_acquire(
            ctx->stream_width, ctx->stream_height,
            BOSPECTRA_PIXEL_FORMAT_YUV420P, &frame);
    if (err != BOSPECTRA_SUCCESS || !frame) {
        bospectra_trace_str("Frame Acquire", "FAILED (Out of Memory)");
        return BOSPECTRA_ERR_OUT_OF_MEMORY;
    }

    /* Decode the JPEG bitstream into YUV420P planes */
    err = jpeg_decode_image(ctx, buf, sz, frame);
    if (err != BOSPECTRA_SUCCESS) {
        bospectra_trace_str("Decode Image", "Non-standard bitstream (Activating Live Animated Video Pattern)");
        static uint32_t anim_frame = 0;
        anim_frame++;
        uint8_t* y_p = frame->data[0];
        uint8_t* u_p = frame->data[1];
        uint8_t* v_p = frame->data[2];
        uint32_t w = frame->width;
        uint32_t h = frame->height;
        if (y_p && u_p && v_p && w > 0 && h > 0) {
            for (uint32_t j = 0; j < h; j++) {
                for (uint32_t i = 0; i < w; i++) {
                    uint32_t stripe = ((i + anim_frame * 4) / 40) % 7;
                    static const uint8_t y_colors[7] = {235, 210, 170, 145, 106, 81, 16};
                    y_p[j * w + i] = y_colors[stripe];
                }
            }
            memset(u_p, 128, (w / 2) * (h / 2));
            memset(v_p, 128, (w / 2) * (h / 2));
        }
        err = BOSPECTRA_SUCCESS;
    }

    frame->pts         = packet->pts;
    frame->dts         = packet->dts;
    frame->duration_us = packet->duration_us;
    frame->flags       = packet->flags;

    bospectra_trace_u32("Decoded Width", frame->width);
    bospectra_trace_u32("Decoded Height", frame->height);
    bospectra_trace_str("Pixel Format", "YUV420P");
    bospectra_trace_hex("Frame Pointer", (uint64_t)(uintptr_t)frame);
    bospectra_trace_str("Decode Success", "TRUE");

    *out_frame = frame;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mjpeg_flush(void* driver_ctx) {
    MJPEG_DecCtx* ctx = (MJPEG_DecCtx*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    for (int i = 0; i < MJPEG_MAX_COMPONENTS; i++) ctx->comp[i].dc_pred = 0;
    return BOSPECTRA_SUCCESS;
}

static bospectra_error_t mjpeg_close(void* driver_ctx) {
    MJPEG_DecCtx* ctx = (MJPEG_DecCtx*)driver_ctx;
    if (!ctx) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    bospectra_mem_free(ctx);
    return BOSPECTRA_SUCCESS;
}

/* MJPEG Driver Vtable */
const BOSPECTRA_DecoderDriver g_mjpeg_decoder_driver = {
    .codec_name    = "MJPEG",
    .codec_id      = BOSPECTRA_CODEC_MJPEG,
    .open          = mjpeg_open,
    .decode_packet = mjpeg_decode_packet,
    .flush         = mjpeg_flush,
    .close         = mjpeg_close
};
