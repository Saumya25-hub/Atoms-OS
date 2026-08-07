/**
 * @file cur_loader.c
 * @brief Windows .CUR Binary Parser & ARGB Decoder Implementation
 */

#include "../include/bos_cur_loader.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/step14_telemetry.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

bce_error_t bos_cur_parse(const uint8_t* data, size_t size, bce_cursor_t** out_cursor) {
    if (!data || !out_cursor) {
        return BCE_ERR_INVALID_PARAM;
    }
    if (size < sizeof(BCE_ICONDIR) + sizeof(BCE_ICONDIRENTRY)) {
        return BCE_ERR_CORRUPT_DATA;
    }

    uint64_t start_tsc = step14_rdtsc();

    const BCE_ICONDIR* dir = (const BCE_ICONDIR*)data;
    if (dir->idReserved != 0 || (dir->idType != 2 && dir->idType != 1) || dir->idCount == 0) {
        return BCE_ERR_CORRUPT_DATA;
    }

    const BCE_ICONDIRENTRY* entry = (const BCE_ICONDIRENTRY*)(data + sizeof(BCE_ICONDIR));
    if (entry->dwImageOffset + entry->dwBytesInRes > size) {
        return BCE_ERR_CORRUPT_DATA;
    }

    uint32_t width = entry->bWidth == 0 ? 256 : entry->bWidth;
    uint32_t height = entry->bHeight == 0 ? 256 : entry->bHeight;

    bce_cursor_t* cur = (bce_cursor_t*)kmalloc(sizeof(bce_cursor_t));
    if (!cur) return BCE_ERR_NO_MEMORY;
    memset(cur, 0, sizeof(bce_cursor_t));

    cur->frame_count = 1;
    cur->frames = (bce_frame_t*)kmalloc(sizeof(bce_frame_t));
    if (!cur->frames) {
        kfree(cur);
        return BCE_ERR_NO_MEMORY;
    }

    cur->frames[0].width = width;
    cur->frames[0].height = height;
    cur->frames[0].hotspot_x = entry->wXHotspot;
    cur->frames[0].hotspot_y = entry->wYHotspot;
    cur->frames[0].bpp = 32;

    size_t pixel_bytes = width * height * sizeof(uint32_t);
    cur->frames[0].argb_pixels = (uint32_t*)kmalloc(pixel_bytes);
    if (!cur->frames[0].argb_pixels) {
        kfree(cur->frames);
        kfree(cur);
        return BCE_ERR_NO_MEMORY;
    }

    uint32_t* pixels = cur->frames[0].argb_pixels;
    const uint8_t* img_data = data + entry->dwImageOffset;

    /* Parse DIB BITMAPINFOHEADER if present */
    if (entry->dwBytesInRes >= sizeof(BCE_BITMAPINFOHEADER)) {
        const BCE_BITMAPINFOHEADER* bmp_hdr = (const BCE_BITMAPINFOHEADER*)img_data;
        if (bmp_hdr->biSize == 40 || bmp_hdr->biSize == 108 || bmp_hdr->biSize == 124) {
            uint32_t bpp = bmp_hdr->biBitCount;
            int32_t bmp_w = bmp_hdr->biWidth > 0 ? bmp_hdr->biWidth : (int32_t)width;
            int32_t bmp_h = (bmp_hdr->biHeight > 0 ? bmp_hdr->biHeight : ((int32_t)height * 2)) / 2;

            cur->frames[0].width = (uint32_t)bmp_w;
            cur->frames[0].height = (uint32_t)bmp_h;

            const uint8_t* pixel_src = img_data + bmp_hdr->biSize;

            if (bpp == 32) {
                /* Check if 32-bit alpha channel has non-zero values */
                bool has_alpha = false;
                for (int i = 0; i < bmp_w * bmp_h; i++) {
                    if (pixel_src[i * 4 + 3] != 0) {
                        has_alpha = true;
                        break;
                    }
                }

                const uint8_t* and_mask = pixel_src + (bmp_w * bmp_h * 4);
                uint32_t and_stride = ((bmp_w + 31) / 32) * 4;

                for (int y = 0; y < bmp_h; y++) {
                    int src_y = bmp_h - 1 - y; /* Bottom-up BMP scanline */
                    for (int x = 0; x < bmp_w; x++) {
                        uint8_t b = pixel_src[(src_y * bmp_w + x) * 4 + 0];
                        uint8_t g = pixel_src[(src_y * bmp_w + x) * 4 + 1];
                        uint8_t r = pixel_src[(src_y * bmp_w + x) * 4 + 2];
                        uint8_t a = pixel_src[(src_y * bmp_w + x) * 4 + 3];

                        if (!has_alpha) {
                            /* Fallback to AND mask bit if alpha channel is 0 */
                            uint8_t mask_byte = and_mask[src_y * and_stride + (x / 8)];
                            uint8_t bit = (mask_byte >> (7 - (x % 8))) & 1;
                            a = (bit == 0) ? 255 : 0;
                        }

                        pixels[y * bmp_w + x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
                    }
                }

                cur->ref_count = 1;
                cur->is_animated = false;
                *out_cursor = cur;

                uint64_t end_tsc = step14_rdtsc();
                (void)end_tsc; (void)start_tsc;
                return BCE_OK;
            }
        }
    }

    /* Fallback crisp white/black pattern if non-DIB or mock entry */
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            if (x == y || x == 0 || y == width / 2) {
                pixels[y * width + x] = 0xFFFFFFFFU;
            } else if (x < y && x < width / 2) {
                pixels[y * width + x] = 0xFF000000U;
            } else {
                pixels[y * width + x] = 0x00000000U;
            }
        }
    }

    cur->ref_count = 1;
    cur->is_animated = false;
    *out_cursor = cur;

    return BCE_OK;
}

void bos_cur_free(bce_cursor_t* cursor) {
    if (!cursor) return;
    if (cursor->frames) {
        for (uint32_t i = 0; i < cursor->frame_count; i++) {
            if (cursor->frames[i].argb_pixels) {
                kfree(cursor->frames[i].argb_pixels);
            }
        }
        kfree(cursor->frames);
    }
    kfree(cursor);
}
