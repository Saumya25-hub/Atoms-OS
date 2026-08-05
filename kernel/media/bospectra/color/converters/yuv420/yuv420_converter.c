#include "yuv420_converter.h"
#include "../../include/bospectra_color_spaces.h"
#include "../../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

static inline uint8_t deblock_y_sample(const uint8_t* y_plane, uint32_t i, uint32_t j, uint32_t width, uint32_t height, uint32_t y_stride) {
    (void)width; (void)height;
    return y_plane[j * y_stride + i];
}

bospectra_error_t bospectra_convert_yuv420p_to_argb32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space) {
    if (!src || !dst || !src->data[0] || !src->data[1] || !src->data[2] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }
    if (src->width == 0 || src->height == 0 || dst->width < src->width || dst->height < src->height) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint8_t* y_plane = src->data[0];
    const uint8_t* u_plane = src->data[1];
    const uint8_t* v_plane = src->data[2];

    uint32_t y_stride = src->linesize[0] ? src->linesize[0] : width;
    uint32_t u_stride = src->linesize[1] ? src->linesize[1] : (width / 2);
    uint32_t v_stride = src->linesize[2] ? src->linesize[2] : (width / 2);

    uint32_t* dst_pixels = (uint32_t*)dst->data[0];
    uint32_t dst_stride = dst->linesize[0] / 4; // Stride in 32-bit uint32 pixels

    for (uint32_t j = 0; j < height; j++) {
        const uint8_t* u_row = u_plane + ((j / 2) * u_stride);
        const uint8_t* v_row = v_plane + ((j / 2) * v_stride);
        uint32_t* dst_row = dst_pixels + (j * dst_stride);

        for (uint32_t i = 0; i < width; i++) {
            uint8_t y_val = deblock_y_sample(y_plane, i, j, width, height, y_stride);
            uint8_t u_val = u_row[i / 2];
            uint8_t v_val = v_row[i / 2];

            uint8_t r, g, b;
            if (color_space == BOSPECTRA_COLOR_SPACE_BT709) {
                bospectra_yuv_to_rgb_bt709(y_val, u_val, v_val, &r, &g, &b);
            } else {
                bospectra_yuv_to_rgb_bt601(y_val, u_val, v_val, &r, &g, &b);
            }

            // ARGB32 Packing: (0xFF000000) | (R << 16) | (G << 8) | B
            dst_row[i] = (0xFF000000U) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }

    dst->width = width;
    dst->height = height;
    dst->format = BOSPECTRA_PIXEL_FORMAT_ARGB32;
    dst->pts = src->pts;
    dst->dts = src->dts;
    dst->flags = src->flags;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_convert_yuv420p_to_rgba32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space) {
    if (!src || !dst || !src->data[0] || !src->data[1] || !src->data[2] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint8_t* y_plane = src->data[0];
    const uint8_t* u_plane = src->data[1];
    const uint8_t* v_plane = src->data[2];

    uint32_t y_stride = src->linesize[0] ? src->linesize[0] : width;
    uint32_t u_stride = src->linesize[1] ? src->linesize[1] : (width / 2);
    uint32_t v_stride = src->linesize[2] ? src->linesize[2] : (width / 2);

    uint32_t* dst_pixels = (uint32_t*)dst->data[0];
    uint32_t dst_stride = dst->linesize[0] / 4;

    for (uint32_t j = 0; j < height; j++) {
        const uint8_t* y_row = y_plane + (j * y_stride);
        const uint8_t* u_row = u_plane + ((j / 2) * u_stride);
        const uint8_t* v_row = v_plane + ((j / 2) * v_stride);
        uint32_t* dst_row = dst_pixels + (j * dst_stride);

        for (uint32_t i = 0; i < width; i++) {
            uint8_t y_val = y_row[i];
            uint8_t u_val = u_row[i / 2];
            uint8_t v_val = v_row[i / 2];

            uint8_t r, g, b;
            if (color_space == BOSPECTRA_COLOR_SPACE_BT709) {
                bospectra_yuv_to_rgb_bt709(y_val, u_val, v_val, &r, &g, &b);
            } else {
                bospectra_yuv_to_rgb_bt601(y_val, u_val, v_val, &r, &g, &b);
            }

            // RGBA32 Packing: (R << 24) | (G << 16) | (B << 8) | 0xFF
            dst_row[i] = ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | 0xFFU;
        }
    }

    dst->format = BOSPECTRA_PIXEL_FORMAT_RGBA32;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_convert_yuv420p_to_rgb24(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space) {
    if (!src || !dst || !src->data[0] || !src->data[1] || !src->data[2] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint8_t* y_plane = src->data[0];
    const uint8_t* u_plane = src->data[1];
    const uint8_t* v_plane = src->data[2];

    uint32_t y_stride = src->linesize[0] ? src->linesize[0] : width;
    uint32_t u_stride = src->linesize[1] ? src->linesize[1] : (width / 2);
    uint32_t v_stride = src->linesize[2] ? src->linesize[2] : (width / 2);

    uint8_t* dst_bytes = dst->data[0];
    uint32_t dst_stride = dst->linesize[0];

    for (uint32_t j = 0; j < height; j++) {
        const uint8_t* y_row = y_plane + (j * y_stride);
        const uint8_t* u_row = u_plane + ((j / 2) * u_stride);
        const uint8_t* v_row = v_plane + ((j / 2) * v_stride);
        uint8_t* dst_row = dst_bytes + (j * dst_stride);

        for (uint32_t i = 0; i < width; i++) {
            uint8_t y_val = y_row[i];
            uint8_t u_val = u_row[i / 2];
            uint8_t v_val = v_row[i / 2];

            uint8_t r, g, b;
            if (color_space == BOSPECTRA_COLOR_SPACE_BT709) {
                bospectra_yuv_to_rgb_bt709(y_val, u_val, v_val, &r, &g, &b);
            } else {
                bospectra_yuv_to_rgb_bt601(y_val, u_val, v_val, &r, &g, &b);
            }

            dst_row[i * 3 + 0] = r;
            dst_row[i * 3 + 1] = g;
            dst_row[i * 3 + 2] = b;
        }
    }

    dst->format = BOSPECTRA_PIXEL_FORMAT_RGB24;
    return BOSPECTRA_SUCCESS;
}
