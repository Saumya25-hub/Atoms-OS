#ifndef BOSPECTRA_COLOR_SPACES_H
#define BOSPECTRA_COLOR_SPACES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BOSPECTRA_COLOR_SPACE_BT601 = 0,    // SDTV / JFIF Full-Range JPEG Standard
    BOSPECTRA_COLOR_SPACE_BT709,        // HDTV Standard (HD Video >= 720p)
    BOSPECTRA_COLOR_SPACE_BT2020       // UHD / HDR Standard
} bospectra_color_space_t;

// Scientific Clamping Function: Restricts values strictly to [0, 255]
static inline uint8_t bospectra_clamp_u8(int32_t val) {
    if (val < 0) return 0;
    if (val > 255) return 255;
    return (uint8_t)val;
}

// JFIF / BT.601 Full-Range YCbCr to RGB (Standard JPEG color conversion)
static inline void bospectra_yuv_to_rgb_bt601(uint8_t y, uint8_t u, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b) {
    int32_t c = (int32_t)y;
    int32_t d = (int32_t)u - 128;
    int32_t e = (int32_t)v - 128;

    int32_t r_val = c + ((1402 * e) >> 10);
    int32_t g_val = c - ((344 * d + 714 * e) >> 10);
    int32_t b_val = c + ((1772 * d) >> 10);

    *r = bospectra_clamp_u8(r_val);
    *g = bospectra_clamp_u8(g_val);
    *b = bospectra_clamp_u8(b_val);
}

// BT.709 YUV to RGB Integer Conversion
static inline void bospectra_yuv_to_rgb_bt709(uint8_t y, uint8_t u, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b) {
    int32_t c = (int32_t)y;
    int32_t d = (int32_t)u - 128;
    int32_t e = (int32_t)v - 128;

    int32_t r_val = c + ((1574 * e) >> 10);
    int32_t g_val = c - ((187 * d + 468 * e) >> 10);
    int32_t b_val = c + ((1856 * d) >> 10);

    *r = bospectra_clamp_u8(r_val);
    *g = bospectra_clamp_u8(g_val);
    *b = bospectra_clamp_u8(b_val);
}

#endif // BOSPECTRA_COLOR_SPACES_H
