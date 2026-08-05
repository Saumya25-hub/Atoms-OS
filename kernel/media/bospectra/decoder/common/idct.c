/*
 * BOSPECTRA V3 — ISO/IEC 10918-1 AAN Integer 8x8 IDCT Implementation
 * kernel/media/bospectra/decoder/common/idct.c
 */

#include "idct.h"
#include "../../color/include/bospectra_color_spaces.h"

#define FIX_0_298631336  2444  /* FIX(0.298631336) */
#define FIX_0_390180644  3196  /* FIX(0.390180644) */
#define FIX_0_541196100  4433  /* FIX(0.541196100) */
#define FIX_0_765366865  6270  /* FIX(0.765366865) */
#define FIX_0_899976223  7373  /* FIX(0.899976223) */
#define FIX_1_175875602  9633  /* FIX(1.175875602) */
#define FIX_1_501321110 12299  /* FIX(1.501321110) */
#define FIX_1_847759065 15137  /* FIX(1.847759065) */

#define DESCALE(x, n)  (((x) + (1 << ((n)-1))) >> (n))

void idct_8x8(const int32_t in_block[64], uint8_t* out_plane, uint32_t stride, uint32_t blk_x, uint32_t blk_y, uint32_t max_h) {
    if (!in_block || !out_plane) return;

    /* Fast DC-Only Block Shortcut (ISO/IEC 10918-1 AAN IDCT) */
    bool is_dc_only = true;
    for (int i = 1; i < 64; i++) {
        if (in_block[i] != 0) { is_dc_only = false; break; }
    }

    if (is_dc_only) {
        int32_t dcval = (in_block[0] >= 0) ? ((in_block[0] + 4) >> 3) : ((in_block[0] - 4) >> 3);
        uint8_t pixel = bospectra_clamp_u8(dcval + 128);
        for (int y = 0; y < 8; y++) {
            if (blk_y + y >= max_h) break;
            uint8_t* row = out_plane + ((blk_y + y) * stride) + blk_x;
            for (int x = 0; x < 8; x++) {
                row[x] = pixel;
            }
        }
        return;
    }

    int32_t workspace[64];

    /* Pass 1: process columns */
    for (int i = 0; i < 8; i++) {
        if (in_block[i+8] == 0 && in_block[i+16] == 0 && in_block[i+24] == 0 &&
            in_block[i+32] == 0 && in_block[i+40] == 0 && in_block[i+48] == 0 && in_block[i+56] == 0) {
            int32_t dcval = in_block[i] << 2;
            for (int j = 0; j < 8; j++) {
                workspace[j*8 + i] = dcval;
            }
            continue;
        }

        int32_t z10 = in_block[i+8] + in_block[i+40];
        int32_t z11 = in_block[i+8] - in_block[i+40];
        int32_t z13 = in_block[i+24] + in_block[i+56];
        int32_t z12 = (in_block[i+24] - in_block[i+56]) * FIX_0_390180644 - z13 * FIX_0_899976223;
        z13 = z13 * FIX_0_298631336 + z12;
        int32_t tmp0 = (in_block[i] + in_block[i+32]) << 13;
        int32_t tmp1 = (in_block[i] - in_block[i+32]) << 13;
        int32_t tmp2 = z10 * FIX_0_541196100 + z11 * FIX_0_765366865;
        int32_t tmp3 = z10 * FIX_1_847759065 - z11 * FIX_1_175875602;

        int32_t tmp10 = tmp0 + tmp3;
        int32_t tmp13 = tmp0 - tmp3;
        int32_t tmp11 = tmp1 + tmp2;
        int32_t tmp12 = tmp1 - tmp2;

        workspace[0*8 + i] = DESCALE(tmp10 + z13, 11);
        workspace[7*8 + i] = DESCALE(tmp10 - z13, 11);
        workspace[1*8 + i] = DESCALE(tmp11 + z12, 11);
        workspace[6*8 + i] = DESCALE(tmp11 - z12, 11);
        workspace[2*8 + i] = DESCALE(tmp12 + z12, 11);
        workspace[5*8 + i] = DESCALE(tmp12 - z12, 11);
        workspace[3*8 + i] = DESCALE(tmp13 + z13, 11);
        workspace[4*8 + i] = DESCALE(tmp13 - z13, 11);
    }

    /* Pass 2: process rows and write to output plane with +128 level shift */
    for (int y = 0; y < 8; y++) {
        if (blk_y + y >= max_h) break;
        uint8_t* row = out_plane + ((blk_y + y) * stride) + blk_x;
        const int32_t* ws = workspace + y*8;

        int32_t z10 = ws[1] + ws[5];
        int32_t z11 = ws[1] - ws[5];
        int32_t z13 = ws[3] + ws[7];
        int32_t z12 = (ws[3] - ws[7]) * FIX_0_390180644 - z13 * FIX_0_899976223;
        z13 = z13 * FIX_0_298631336 + z12;

        int32_t tmp0 = (ws[0] + ws[4]) << 13;
        int32_t tmp1 = (ws[0] - ws[4]) << 13;
        int32_t tmp2 = z10 * FIX_0_541196100 + z11 * FIX_0_765366865;
        int32_t tmp3 = z10 * FIX_1_847759065 - z11 * FIX_1_175875602;

        int32_t tmp10 = tmp0 + tmp3;
        int32_t tmp13 = tmp0 - tmp3;
        int32_t tmp11 = tmp1 + tmp2;
        int32_t tmp12 = tmp1 - tmp2;

        row[0] = bospectra_clamp_u8(DESCALE(tmp10 + z13, 18) + 128);
        row[7] = bospectra_clamp_u8(DESCALE(tmp10 - z13, 18) + 128);
        row[1] = bospectra_clamp_u8(DESCALE(tmp11 + z12, 18) + 128);
        row[6] = bospectra_clamp_u8(DESCALE(tmp11 - z12, 18) + 128);
        row[2] = bospectra_clamp_u8(DESCALE(tmp12 + z12, 18) + 128);
        row[5] = bospectra_clamp_u8(DESCALE(tmp12 - z12, 18) + 128);
        row[3] = bospectra_clamp_u8(DESCALE(tmp13 + z13, 18) + 128);
        row[4] = bospectra_clamp_u8(DESCALE(tmp13 - z13, 18) + 128);
    }
}
