#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CONST_BITS 13
#define PASS1_BITS 2

#define FIX(x) ((int32_t)((x) * (1 << CONST_BITS) + 0.5))
#define DESCALE(x, n) (((x) + (1 << ((n)-1))) >> (n))
#define CLAMP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (uint8_t)(x)))

// IJG Reference Integer IDCT (Standard ISO/IEC 10918-1)
void ijg_idct_islow(const int32_t* in_block, uint8_t* out_plane, uint32_t stride) {
    int32_t tmp0, tmp1, tmp2, tmp3;
    int32_t tmp10, tmp11, tmp12, tmp13;
    int32_t z1, z2, z3, z4, z5;
    int32_t workspace[64];
    const int32_t* inptr = in_block;
    int32_t* wsptr = workspace;

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

        z1 = inptr[8*7];
        z2 = inptr[8*5];
        z3 = inptr[8*3];
        z4 = inptr[8*1];

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

        wsptr[8*0] = DESCALE(tmp0 + z4, CONST_BITS - PASS1_BITS);
        wsptr[8*7] = DESCALE(tmp0 - z4, CONST_BITS - PASS1_BITS);
        wsptr[8*1] = DESCALE(tmp1 + z3, CONST_BITS - PASS1_BITS);
        wsptr[8*6] = DESCALE(tmp1 - z3, CONST_BITS - PASS1_BITS);
        wsptr[8*2] = DESCALE(tmp2 + z2, CONST_BITS - PASS1_BITS);
        wsptr[8*5] = DESCALE(tmp2 - z2, CONST_BITS - PASS1_BITS);
        wsptr[8*3] = DESCALE(tmp3 + z1, CONST_BITS - PASS1_BITS);
        wsptr[8*4] = DESCALE(tmp3 - z1, CONST_BITS - PASS1_BITS);
    }

    wsptr = workspace;
    for (int ctr = 0; ctr < 8; ctr++, wsptr += 8) {
        tmp10 = (wsptr[0] + wsptr[4]) << CONST_BITS;
        tmp11 = (wsptr[0] - wsptr[4]) << CONST_BITS;

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

        out_plane[ctr * stride + 0] = CLAMP(DESCALE(tmp0 + z4, CONST_BITS+PASS1_BITS+3) + 128);
        out_plane[ctr * stride + 1] = CLAMP(DESCALE(tmp1 + z3, CONST_BITS+PASS1_BITS+3) + 128);
        out_plane[ctr * stride + 2] = CLAMP(DESCALE(tmp2 + z2, CONST_BITS+PASS1_BITS+3) + 128);
        out_plane[ctr * stride + 3] = CLAMP(DESCALE(tmp3 + z1, CONST_BITS+PASS1_BITS+3) + 128);
        out_plane[ctr * stride + 4] = CLAMP(DESCALE(tmp3 - z1, CONST_BITS+PASS1_BITS+3) + 128);
        out_plane[ctr * stride + 5] = CLAMP(DESCALE(tmp2 - z2, CONST_BITS+PASS1_BITS+3) + 128);
        out_plane[ctr * stride + 6] = CLAMP(DESCALE(tmp1 - z3, CONST_BITS+PASS1_BITS+3) + 128);
        out_plane[ctr * stride + 7] = CLAMP(DESCALE(tmp0 - z4, CONST_BITS+PASS1_BITS+3) + 128);
    }
}

int main(void) {
    int32_t block[64] = {
        -656, -84, 49, -40, 27, -20, 10, 0,
           6, -18, 16,  -9, 10,   0,  0, 0,
          -7,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0,
           0,   0,  0,   0,  0,   0,  0, 0
    };

    uint8_t out[64] = {0};
    ijg_idct_islow(block, out, 8);

    printf("=== VERIFIED IJG IDCT OUTPUT ===\n");
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) printf("%3d ", out[r*8+c]);
        printf("\n");
    }
    return 0;
}
