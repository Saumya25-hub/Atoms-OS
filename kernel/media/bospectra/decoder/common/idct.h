#ifndef IDCT_H
#define IDCT_H

#include <stdint.h>

// Fast 8x8 Integer Inverse Discrete Cosine Transform
void idct_8x8(const int32_t in_block[64], uint8_t* out_plane, uint32_t stride, uint32_t blk_x, uint32_t blk_y, uint32_t max_h);

#endif // IDCT_H
