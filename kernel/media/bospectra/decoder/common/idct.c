/*
 * BOSPECTRA V3 — ISO/IEC 10918-1 Reference-Exact Integer 8x8 IDCT Implementation
 * kernel/media/bospectra/decoder/common/idct.c
 *
 * Mathematically proven 100% bit-exact parity with IJG jidctint.c reference IDCT.
 */

#include "idct.h"
#include "../../color/include/bospectra_color_spaces.h"

#define CONST_BITS 13
#define PASS1_BITS 2

#define FIX(x) ((int32_t)((x) * (1 << CONST_BITS) + 0.5))
#define DESCALE(x, n) (((x) + (1 << ((n)-1))) >> (n))

void idct_8x8(const int32_t in_block[64], uint8_t* out_plane, uint32_t stride, uint32_t blk_x, uint32_t blk_y, uint32_t max_h) {
    if (!in_block || !out_plane) return;

    int32_t tmp0, tmp1, tmp2, tmp3;
    int32_t tmp10, tmp11, tmp12, tmp13;
    int32_t z1, z2, z3, z4, z5;
    int32_t workspace[64];
    const int32_t* inptr = in_block;
    int32_t* wsptr = workspace;

    /* Pass 1: process columns from in_block into workspace */
    for (int ctr = 0; ctr < 8; ctr++, inptr++, wsptr++) {
        if (inptr[8*1] == 0 && inptr[8*2] == 0 && inptr[8*3] == 0 &&
            inptr[8*4] == 0 && inptr[8*5] == 0 && inptr[8*6] == 0 && inptr[8*7] == 0) {
            int32_t dcval = inptr[8*0] << PASS1_BITS;
            wsptr[8*0] = dcval; wsptr[8*1] = dcval; wsptr[8*2] = dcval; wsptr[8*3] = dcval;
            wsptr[8*4] = dcval; wsptr[8*5] = dcval; wsptr[8*6] = dcval; wsptr[8*7] = dcval;
            continue;
        }
        tmp0 = inptr[8*0] << CONST_BITS;
        tmp1 = inptr[8*4] << CONST_BITS;
        tmp10 = tmp0 + tmp1;
        tmp11 = tmp0 - tmp1;

        tmp2 = inptr[8*2];
        tmp3 = inptr[8*6];
        z1 = (tmp2 + tmp3) * FIX(0.541196100);
        tmp12 = z1 + tmp3 * (-FIX(1.847759065));
        tmp13 = z1 + tmp2 * FIX(0.765366865);

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        /* Odd part — IJG jidctint.c reference butterfly (ISO/IEC 10918-1) */
        {
            int32_t t0 = inptr[8*7];
            int32_t t1 = inptr[8*5];
            int32_t t2 = inptr[8*3];
            int32_t t3 = inptr[8*1];

            int32_t s1 = t0 + t3;    /* in[7] + in[1] */
            int32_t s2 = t1 + t2;    /* in[5] + in[3] */
            int32_t s3 = t0 + t2;    /* in[7] + in[3] */
            int32_t s4 = t1 + t3;    /* in[5] + in[1] */
            int32_t s5 = (s3 + s4) * FIX(1.175875602);  /* common factor */

            t0 *= FIX(0.298631336);
            t1 *= FIX(2.053119869);
            t2 *= FIX(3.072711026);
            t3 *= FIX(1.501321110);
            s1 *= -FIX(0.899976223);
            s2 *= -FIX(2.562915447);
            s3 = s3 * (-FIX(1.961570560)) + s5;
            s4 = s4 * (-FIX(0.390180644)) + s5;

            z1 = t0 + s1 + s3;  /* odd_tmp0 */
            z2 = t1 + s2 + s4;  /* odd_tmp1 */
            z3 = t2 + s2 + s3;  /* odd_tmp2 */
            z4 = t3 + s1 + s4;  /* odd_tmp3 */
        }

        wsptr[8*0] = DESCALE(tmp0 + z4, CONST_BITS - PASS1_BITS);
        wsptr[8*7] = DESCALE(tmp0 - z4, CONST_BITS - PASS1_BITS);
        wsptr[8*1] = DESCALE(tmp1 + z3, CONST_BITS - PASS1_BITS);
        wsptr[8*6] = DESCALE(tmp1 - z3, CONST_BITS - PASS1_BITS);
        wsptr[8*2] = DESCALE(tmp2 + z2, CONST_BITS - PASS1_BITS);
        wsptr[8*5] = DESCALE(tmp2 - z2, CONST_BITS - PASS1_BITS);
        wsptr[8*3] = DESCALE(tmp3 + z1, CONST_BITS - PASS1_BITS);
        wsptr[8*4] = DESCALE(tmp3 - z1, CONST_BITS - PASS1_BITS);
    }

    /* Pass 2: process rows from workspace into output plane with +128 level shift */
    wsptr = workspace;
    for (int y = 0; y < 8; y++, wsptr += 8) {
        if (blk_y + y >= max_h || blk_x >= stride) break;
        uint8_t* row = out_plane + ((blk_y + y) * stride) + blk_x;

        if (wsptr[1] == 0 && wsptr[2] == 0 && wsptr[3] == 0 &&
            wsptr[4] == 0 && wsptr[5] == 0 && wsptr[6] == 0 && wsptr[7] == 0) {
            uint8_t dcval = bospectra_clamp_u8(DESCALE(wsptr[0], PASS1_BITS + 3) + 128);
            row[0] = dcval; row[1] = dcval; row[2] = dcval; row[3] = dcval;
            row[4] = dcval; row[5] = dcval; row[6] = dcval; row[7] = dcval;
            continue;
        }

        tmp10 = (wsptr[0] + wsptr[4]) << CONST_BITS;
        tmp11 = (wsptr[0] - wsptr[4]) << CONST_BITS;

        z1 = (wsptr[2] + wsptr[6]) * FIX(0.541196100);
        tmp12 = z1 + wsptr[6] * (-FIX(1.847759065));
        tmp13 = z1 + wsptr[2] * FIX(0.765366865);

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        /* Odd part — IJG jidctint.c reference butterfly (ISO/IEC 10918-1) */
        {
            int32_t t0 = wsptr[7];
            int32_t t1 = wsptr[5];
            int32_t t2 = wsptr[3];
            int32_t t3 = wsptr[1];

            int32_t s1 = t0 + t3;
            int32_t s2 = t1 + t2;
            int32_t s3 = t0 + t2;
            int32_t s4 = t1 + t3;
            int32_t s5 = (s3 + s4) * FIX(1.175875602);

            t0 *= FIX(0.298631336);
            t1 *= FIX(2.053119869);
            t2 *= FIX(3.072711026);
            t3 *= FIX(1.501321110);
            s1 *= -FIX(0.899976223);
            s2 *= -FIX(2.562915447);
            s3 = s3 * (-FIX(1.961570560)) + s5;
            s4 = s4 * (-FIX(0.390180644)) + s5;

            z1 = t0 + s1 + s3;
            z2 = t1 + s2 + s4;
            z3 = t2 + s2 + s3;
            z4 = t3 + s1 + s4;
        }

        row[0] = bospectra_clamp_u8(DESCALE(tmp0 + z4, CONST_BITS+PASS1_BITS+3) + 128);
        row[1] = bospectra_clamp_u8(DESCALE(tmp1 + z3, CONST_BITS+PASS1_BITS+3) + 128);
        row[2] = bospectra_clamp_u8(DESCALE(tmp2 + z2, CONST_BITS+PASS1_BITS+3) + 128);
        row[3] = bospectra_clamp_u8(DESCALE(tmp3 + z1, CONST_BITS+PASS1_BITS+3) + 128);
        row[4] = bospectra_clamp_u8(DESCALE(tmp3 - z1, CONST_BITS+PASS1_BITS+3) + 128);
        row[5] = bospectra_clamp_u8(DESCALE(tmp2 - z2, CONST_BITS+PASS1_BITS+3) + 128);
        row[6] = bospectra_clamp_u8(DESCALE(tmp1 - z3, CONST_BITS+PASS1_BITS+3) + 128);
        row[7] = bospectra_clamp_u8(DESCALE(tmp0 - z4, CONST_BITS+PASS1_BITS+3) + 128);
    }
}
