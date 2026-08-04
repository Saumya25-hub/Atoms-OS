#include "yuy2_converter.h"
#include "../../include/bospectra_color_spaces.h"
#include "../../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

bospectra_error_t bospectra_convert_yuy2_to_argb32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space) {
    if (!src || !dst || !src->data[0] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint8_t* src_bytes = src->data[0]; // Packed YUYV (4 bytes per 2 pixels: Y0, U0, Y1, V0)
    uint32_t src_stride = src->linesize[0] ? src->linesize[0] : (width * 2);

    uint32_t* dst_pixels = (uint32_t*)dst->data[0];
    uint32_t dst_stride = dst->linesize[0] / 4;

    for (uint32_t j = 0; j < height; j++) {
        const uint8_t* src_row = src_bytes + (j * src_stride);
        uint32_t* dst_row = dst_pixels + (j * dst_stride);

        for (uint32_t i = 0; i < width; i += 2) {
            uint8_t y0 = src_row[i * 2 + 0];
            uint8_t u0 = src_row[i * 2 + 1];
            uint8_t y1 = src_row[i * 2 + 2];
            uint8_t v0 = src_row[i * 2 + 3];

            uint8_t r0, g0, b0, r1, g1, b1;
            if (color_space == BOSPECTRA_COLOR_SPACE_BT709) {
                bospectra_yuv_to_rgb_bt709(y0, u0, v0, &r0, &g0, &b0);
                bospectra_yuv_to_rgb_bt709(y1, u0, v0, &r1, &g1, &b1);
            } else {
                bospectra_yuv_to_rgb_bt601(y0, u0, v0, &r0, &g0, &b0);
                bospectra_yuv_to_rgb_bt601(y1, u0, v0, &r1, &g1, &b1);
            }

            dst_row[i + 0] = (0xFF000000U) | ((uint32_t)r0 << 16) | ((uint32_t)g0 << 8) | (uint32_t)b0;
            dst_row[i + 1] = (0xFF000000U) | ((uint32_t)r1 << 16) | ((uint32_t)g1 << 8) | (uint32_t)b1;
        }
    }

    dst->width = width;
    dst->height = height;
    dst->format = BOSPECTRA_PIXEL_FORMAT_ARGB32;
    dst->pts = src->pts;

    return BOSPECTRA_SUCCESS;
}
