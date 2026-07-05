#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/surface/surface.h"
#include <stddef.h>

extern uint32_t crc32(const uint8_t *buf, int len);
extern int inflate_decompress(const uint8_t* in_data, uint32_t in_size, uint8_t* out_data, uint32_t out_capacity, uint32_t* out_size);
extern void png_unfilter_scanline(uint8_t filter_type, uint8_t* line, const uint8_t* prev_line, int bytes_per_pixel, int stride);

static uint32_t read_u32(const uint8_t* buf) {
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

struct BOSSurface* png_decode(const uint8_t* buffer, uint32_t size) {
    if (size < 33) return NULL;
    
    // Check signature
    if (buffer[0] != 0x89 || buffer[1] != 0x50 || buffer[2] != 0x4E || buffer[3] != 0x47) return NULL;
    
    int w = 0, h = 0;
    uint8_t bit_depth = 0;
    uint8_t color_type = 0;
    
    uint32_t offset = 8;
    
    // Allocate buffer for compressed IDAT data
    uint32_t idat_cap = size;
    uint8_t* idat_data = (uint8_t*)kmalloc(idat_cap);
    if (!idat_data) return NULL;
    uint32_t idat_size = 0;
    
    // Palette support
    uint8_t palette[768] = {0};
    int has_palette = 0;
    
    while (offset < size) {
        if (offset + 8 > size) break;
        uint32_t chunk_length = read_u32(buffer + offset);
        const uint8_t* chunk_type = buffer + offset + 4;
        const uint8_t* chunk_data = buffer + offset + 8;
        if (offset + 12 + chunk_length > size) break;
        
        if (chunk_type[0] == 'I' && chunk_type[1] == 'H' && chunk_type[2] == 'D' && chunk_type[3] == 'R') {
            w = read_u32(chunk_data);
            h = read_u32(chunk_data + 4);
            bit_depth = chunk_data[8];
            color_type = chunk_data[9];
        } else if (chunk_type[0] == 'P' && chunk_type[1] == 'L' && chunk_type[2] == 'T' && chunk_type[3] == 'E') {
            for (uint32_t i = 0; i < chunk_length && i < 768; i++) {
                palette[i] = chunk_data[i];
            }
            has_palette = 1;
        } else if (chunk_type[0] == 'I' && chunk_type[1] == 'D' && chunk_type[2] == 'A' && chunk_type[3] == 'T') {
            if (idat_size + chunk_length <= idat_cap) {
                for (uint32_t i = 0; i < chunk_length; i++) idat_data[idat_size++] = chunk_data[i];
            }
        } else if (chunk_type[0] == 'I' && chunk_type[1] == 'E' && chunk_type[2] == 'N' && chunk_type[3] == 'D') {
            break;
        }
        
        offset += 12 + chunk_length;
    }
    
    if (w <= 0 || h <= 0 || idat_size == 0) {
        kfree(idat_data);
        return NULL;
    }
    
    int bytes_per_pixel = 1;
    if (color_type == 2) bytes_per_pixel = 3;
    else if (color_type == 4) bytes_per_pixel = 2;
    else if (color_type == 6) bytes_per_pixel = 4;
    
    uint32_t uncompressed_size = (w * bytes_per_pixel + 1) * h;
    uint8_t* uncompressed_data = (uint8_t*)kmalloc(uncompressed_size);
    if (!uncompressed_data) {
        kfree(idat_data);
        return NULL;
    }
    
    uint32_t out_size = 0;
    int res = inflate_decompress(idat_data, idat_size, uncompressed_data, uncompressed_size, &out_size);
    kfree(idat_data);
    
    if (res < 0) {
        kfree(uncompressed_data);
        return NULL;
    }
    
    struct BOSSurface* surface = surface_create(w, h);
    if (!surface) {
        kfree(uncompressed_data);
        return NULL;
    }
    
    int stride = w * bytes_per_pixel;
    uint8_t* prev_line = NULL;
    
    for (int y = 0; y < h; y++) {
        uint8_t* line_start = uncompressed_data + y * (stride + 1);
        uint8_t filter = line_start[0];
        uint8_t* line_data = line_start + 1;
        
        png_unfilter_scanline(filter, line_data, prev_line, bytes_per_pixel, stride);
        
        for (int x = 0; x < w; x++) {
            uint32_t pixel = 0xFF000000;
            if (color_type == 2) {
                uint8_t r = line_data[x * 3];
                uint8_t g = line_data[x * 3 + 1];
                uint8_t b = line_data[x * 3 + 2];
                pixel = 0xFF000000 | (r << 16) | (g << 8) | b;
            } else if (color_type == 6) {
                uint8_t r = line_data[x * 4];
                uint8_t g = line_data[x * 4 + 1];
                uint8_t b = line_data[x * 4 + 2];
                uint8_t a = line_data[x * 4 + 3];
                pixel = (a << 24) | (r << 16) | (g << 8) | b;
            } else if (color_type == 3 && has_palette) {
                uint8_t idx = line_data[x];
                uint8_t r = palette[idx * 3];
                uint8_t g = palette[idx * 3 + 1];
                uint8_t b = palette[idx * 3 + 2];
                pixel = 0xFF000000 | (r << 16) | (g << 8) | b;
            } else if (color_type == 0) {
                uint8_t g = line_data[x];
                pixel = 0xFF000000 | (g << 16) | (g << 8) | g;
            } else if (color_type == 4) {
                uint8_t g = line_data[x * 2];
                uint8_t a = line_data[x * 2 + 1];
                pixel = (a << 24) | (g << 16) | (g << 8) | g;
            }
            surface->framebuffer[y * w + x] = pixel;
        }
        prev_line = line_data;
    }
    
    kfree(uncompressed_data);
    return surface;
}
