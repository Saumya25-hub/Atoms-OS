#ifndef IDCT_H
#define IDCT_H

#include <stdint.h>

// Fast 8x8 Integer Inverse Discrete Cosine Transform
void idct_8x8(const int32_t in_block[64], uint8_t* out_plane, uint32_t stride);

#endif // IDCT_H
