#include "nv12_converter.h"
#include "../../include/bospectra_color_spaces.h"
#include "../../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

bospectra_error_t bospectra_convert_nv12_to_argb32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space) {
    if (!src || !dst || !src->data[0] || !src->data[1] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint8_t* y_plane = src->data[0];
    const uint8_t* uv_plane = src->data[1]; // Interleaved UV (U0, V0, U1, V1, ...)

    uint32_t y_stride = src->linesize[0] ? src->linesize[0] : width;
    uint32_t uv_stride = src->linesize[1] ? src->linesize[1] : width;

    uint32_t* dst_pixels = (uint32_t*)dst->data[0];
    uint32_t dst_stride = dst->linesize[0] / 4;

    for (uint32_t j = 0; j < height; j++) {
        const uint8_t* y_row = y_plane + (j * y_stride);
        const uint8_t* uv_row = uv_plane + ((j / 2) * uv_stride);
        uint32_t* dst_row = dst_pixels + (j * dst_stride);

        for (uint32_t i = 0; i < width; i++) {
            uint8_t y_val = y_row[i];
            uint32_t uv_idx = (i / 2) * 2;
            uint8_t u_val = uv_row[uv_idx];
            uint8_t v_val = uv_row[uv_idx + 1];

            uint8_t r, g, b;
            if (color_space == BOSPECTRA_COLOR_SPACE_BT709) {
                bospectra_yuv_to_rgb_bt709(y_val, u_val, v_val, &r, &g, &b);
            } else {
                bospectra_yuv_to_rgb_bt601(y_val, u_val, v_val, &r, &g, &b);
            }

            dst_row[i] = (0xFF000000U) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }

    dst->width = width;
    dst->height = height;
    dst->format = BOSPECTRA_PIXEL_FORMAT_ARGB32;
    dst->pts = src->pts;
    dst->dts = src->dts;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_convert_nv12_to_rgba32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space) {
    if (!src || !dst || !src->data[0] || !src->data[1] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint8_t* y_plane = src->data[0];
    const uint8_t* uv_plane = src->data[1];

    uint32_t y_stride = src->linesize[0] ? src->linesize[0] : width;
    uint32_t uv_stride = src->linesize[1] ? src->linesize[1] : width;

    uint32_t* dst_pixels = (uint32_t*)dst->data[0];
    uint32_t dst_stride = dst->linesize[0] / 4;

    for (uint32_t j = 0; j < height; j++) {
        const uint8_t* y_row = y_plane + (j * y_stride);
        const uint8_t* uv_row = uv_plane + ((j / 2) * uv_stride);
        uint32_t* dst_row = dst_pixels + (j * dst_stride);

        for (uint32_t i = 0; i < width; i++) {
            uint8_t y_val = y_row[i];
            uint32_t uv_idx = (i / 2) * 2;
            uint8_t u_val = uv_row[uv_idx];
            uint8_t v_val = uv_row[uv_idx + 1];

            uint8_t r, g, b;
            if (color_space == BOSPECTRA_COLOR_SPACE_BT709) {
                bospectra_yuv_to_rgb_bt709(y_val, u_val, v_val, &r, &g, &b);
            } else {
                bospectra_yuv_to_rgb_bt601(y_val, u_val, v_val, &r, &g, &b);
            }

            dst_row[i] = ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | 0xFFU;
        }
    }

    dst->format = BOSPECTRA_PIXEL_FORMAT_RGBA32;
    return BOSPECTRA_SUCCESS;
}
