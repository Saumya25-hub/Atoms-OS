#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "kernel/media/bospectra/decoder/common/idct.h"

// IJG jidctint.c (Integer Slow/Standard IDCT) Reference Implementation
#define CONST_BITS 13
#define PASS1_BITS 2

#define FIX(x) ((int32_t)((x) * (1 << CONST_BITS) + 0.5))
#define DESCALE(x,n) (((x) + (1 << ((n)-1))) >> (n))

static void ijg_idct_islow(const int32_t* in_block, uint8_t* out_plane, uint32_t stride) {
    int32_t tmp0, tmp1, tmp2, tmp3;
    int32_t tmp10, tmp11, tmp12, tmp13;
    int32_t z1, z2, z3, z4, z5;
    int32_t workspace[64];
    const int32_t* inptr = in_block;
    int32_t* wsptr = workspace;

    /* Pass 1: process columns */
    for (int ctr = 0; ctr < 8; ctr++, inptr++, wsptr++) {
        if (inptr[8*1] == 0 && inptr[8*2] == 0 && inptr[8*3] == 0 &&
            inptr[8*4] == 0 && inptr[8*5] == 0 && inptr[8*6] == 0 && inptr[8*7] == 0) {
            int32_t dcval = inptr[8*0] << PASS1_BITS;
            wsptr[8*0] = dcval; wsptr[8*1] = dcval; wsptr[8*2] = dcval; wsptr[8*3] = dcval;
            wsptr[8*4] = dcval; wsptr[8*5] = dcval; wsptr[8*6] = dcval; wsptr[8*7] = dcval;
            continue;
        }
        tmp0 = inptr[8*0] << PASS1_BITS;
        tmp1 = inptr[8*4] << PASS1_BITS;
        tmp10 = tmp0 + tmp1;
        tmp11 = tmp0 - tmp1;

        tmp2 = inptr[8*2] << PASS1_BITS;
        tmp3 = inptr[8*6] << PASS1_BITS;
        z1 = (tmp2 + tmp3) * FIX(0.541196100);
        tmp12 = z1 + tmp3 * (-FIX(1.847759065));
        tmp13 = z1 + tmp2 * FIX(0.765366865);

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        z1 = inptr[8*7] << PASS1_BITS;
        z2 = inptr[8*5] << PASS1_BITS;
        z3 = inptr[8*3] << PASS1_BITS;
        z4 = inptr[8*1] << PASS1_BITS;

        z5 = z1 + z4;
        z1 += z3;
        z2 += z4;
        z3 += z2;
        z4 = z5 + z2;

        z5 = (z3 - z4) * FIX(0.382683432);
        z2 = z1 * (-FIX(0.899976223)) + z5;
        z3 = z3 * (-FIX(2.562915447)) + (z5 - (z4 * FIX(0.382683432)));
        z1 = z1 * FIX(1.501321110) + z5;
        z4 = z4 * FIX(2.053119869) + z5;

        wsptr[8*0] = tmp0 + z4; wsptr[8*7] = tmp0 - z4;
        wsptr[8*1] = tmp1 + z3; wsptr[8*6] = tmp1 - z3;
        wsptr[8*2] = tmp2 + z2; wsptr[8*5] = tmp2 - z2;
        wsptr[8*3] = tmp3 + z1; wsptr[8*4] = tmp3 - z1;
    }

    /* Pass 2: process rows */
    wsptr = workspace;
    for (int ctr = 0; ctr < 8; ctr++, wsptr += 8) {
        tmp10 = wsptr[0] + wsptr[4];
        tmp11 = wsptr[0] - wsptr[4];

        z1 = (wsptr[2] + wsptr[6]) * FIX(0.541196100);
        tmp12 = z1 + wsptr[6] * (-FIX(1.847759065));
        tmp13 = z1 + wsptr[2] * FIX(0.765366865);

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        z1 = wsptr[7]; z2 = wsptr[5]; z3 = wsptr[3]; z4 = wsptr[1];
        z5 = z1 + z4; z1 += z3; z2 += z4; z3 += z2; z4 = z5 + z2;

        z5 = (z3 - z4) * FIX(0.382683432);
        z2 = z1 * (-FIX(0.899976223)) + z5;
        z3 = z3 * (-FIX(2.562915447)) + (z5 - (z4 * FIX(0.382683432)));
        z1 = z1 * FIX(1.501321110) + z5;
        z4 = z4 * FIX(2.053119869) + z5;

        out_plane[ctr * stride + 0] = (uint8_t)DESCALE(tmp0 + z4 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
        out_plane[ctr * stride + 1] = (uint8_t)DESCALE(tmp1 + z3 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
        out_plane[ctr * stride + 2] = (uint8_t)DESCALE(tmp2 + z2 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
        out_plane[ctr * stride + 3] = (uint8_t)DESCALE(tmp3 + z1 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
        out_plane[ctr * stride + 4] = (uint8_t)DESCALE(tmp3 - z1 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
        out_plane[ctr * stride + 5] = (uint8_t)DESCALE(tmp2 - z2 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
        out_plane[ctr * stride + 6] = (uint8_t)DESCALE(tmp1 - z3 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
        out_plane[ctr * stride + 7] = (uint8_t)DESCALE(tmp0 - z4 + (128 << (CONST_BITS+PASS1_BITS+3)), CONST_BITS+PASS1_BITS+3);
    }
}

int main(void) {
    int32_t in_coeff[64] = {
        -656, -84, 49, -40, 27, -20, 10, 0,
           6, -18, 16,  -9, 10,   0,  0, 0,
          -7,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0
    };

    uint8_t bos_out[64];
    uint8_t ijg_out[64];

    memset(bos_out, 0, sizeof(bos_out));
    memset(ijg_out, 0, sizeof(ijg_out));

    idct_8x8(in_coeff, bos_out, 8, 0, 0, 8);
    ijg_idct_islow(in_coeff, ijg_out, 8);

    printf("=== STEP 5-6: IDCT TRANSFORM STAGE COMPARISON (BLOCK 320, 184) ===\n");
    printf("Row | IJG Reference Pixels             | BOSPECTRA IDCT Pixels            | Delta\n");
    printf("----|----------------------------------|----------------------------------|------\n");

    int total_diff = 0;
    for (int r = 0; r < 8; r++) {
        printf(" %d  | ", r);
        for (int c = 0; c < 8; c++) printf("%3d ", ijg_out[r*8+c]);
        printf("| ");
        for (int c = 0; c < 8; c++) printf("%3d ", bos_out[r*8+c]);
        printf("| ");
        for (int c = 0; c < 8; c++) {
            int d = (int)bos_out[r*8+c] - (int)ijg_out[r*8+c];
            total_diff += abs(d);
            printf("%+2d ", d);
        }
        printf("\n");
    }

    printf("\nTotal Cumulative Pixel Delta: %d\n", total_diff);
    if (total_diff == 0) {
        printf("===> CASE C CONFIRMED: idct_8x8 output is 100%% BIT-EXACT MATCH with IJG! Mismatch is in YUV->RGB / Renderer pipeline!\n");
    } else {
        printf("===> CASE B CONFIRMED: idct_8x8 output diverges from IJG! Issue is inside idct.c scaling / descale / AAN constants!\n");
    }

    return 0;
}
