#include <stdio.h>
#include <stdint.h>

void idct_8x8(const int32_t in_block[64], uint8_t* out_plane, uint32_t stride, uint32_t blk_x, uint32_t blk_y, uint32_t max_h);

int main(void) {
    int32_t block[64] = {0};
    block[0] = -8;
    block[8] = -6;
    uint8_t out[64] = {0};
    idct_8x8(block, out, 8, 0, 0, 8);
    printf("BOS IDCT Output for Cb Block (14, 5):\n");
    for (int r=0; r<8; r++) {
        for (int c=0; c<8; c++) printf("%3d ", out[r*8+c]);
        printf("\n");
    }
    return 0;
}
