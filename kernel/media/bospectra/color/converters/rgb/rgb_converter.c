#include "rgb_converter.h"
#include "../../../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

bospectra_error_t bospectra_convert_rgb24_to_argb32(const BOSFrame* src, BOSFrame* dst) {
    if (!src || !dst || !src->data[0] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint8_t* src_bytes = src->data[0];
    uint32_t src_stride = src->linesize[0] ? src->linesize[0] : (width * 3);

    uint32_t* dst_pixels = (uint32_t*)dst->data[0];
    uint32_t dst_stride = dst->linesize[0] / 4;

    for (uint32_t j = 0; j < height; j++) {
        const uint8_t* src_row = src_bytes + (j * src_stride);
        uint32_t* dst_row = dst_pixels + (j * dst_stride);

        for (uint32_t i = 0; i < width; i++) {
            uint8_t r = src_row[i * 3 + 0];
            uint8_t g = src_row[i * 3 + 1];
            uint8_t b = src_row[i * 3 + 2];

            dst_row[i] = (0xFF000000U) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }

    dst->width = width;
    dst->height = height;
    dst->format = BOSPECTRA_PIXEL_FORMAT_ARGB32;
    dst->pts = src->pts;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_swizzle_argb32_to_rgba32(const BOSFrame* src, BOSFrame* dst) {
    if (!src || !dst || !src->data[0] || !dst->data[0]) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    uint32_t width = src->width;
    uint32_t height = src->height;

    const uint32_t* src_pixels = (const uint32_t*)src->data[0];
    uint32_t src_stride = src->linesize[0] / 4;

    uint32_t* dst_pixels = (uint32_t*)dst->data[0];
    uint32_t dst_stride = dst->linesize[0] / 4;

    for (uint32_t j = 0; j < height; j++) {
        const uint32_t* src_row = src_pixels + (j * src_stride);
        uint32_t* dst_row = dst_pixels + (j * dst_stride);

        for (uint32_t i = 0; i < width; i++) {
            uint32_t px = src_row[i];
            uint8_t a = (px >> 24) & 0xFF;
            uint8_t r = (px >> 16) & 0xFF;
            uint8_t g = (px >> 8) & 0xFF;
            uint8_t b = px & 0xFF;

            dst_row[i] = ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a;
        }
    }

    dst->format = BOSPECTRA_PIXEL_FORMAT_RGBA32;
    return BOSPECTRA_SUCCESS;
}
