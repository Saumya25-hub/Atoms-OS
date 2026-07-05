#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/core/memory/heap/include/heap.h"
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_res;
    int32_t y_res;
    uint32_t colors;
    uint32_t imp_colors;
} BMPInfoHeader;
#pragma pack(pop)

struct BOSSurface* bmp_decode(const uint8_t* buffer, uint32_t size) {
    if (size < sizeof(BMPHeader) + sizeof(BMPInfoHeader)) return NULL;
    
    BMPHeader* header = (BMPHeader*)buffer;
    if (header->type != 0x4D42) return NULL; // 'BM'
    
    BMPInfoHeader* info = (BMPInfoHeader*)(buffer + sizeof(BMPHeader));
    
    if (info->bits != 24 && info->bits != 32) return NULL; // Only 24/32 supported
    
    int w = info->width;
    int h = info->height;
    bool bottom_up = true;
    if (h < 0) {
        h = -h;
        bottom_up = false;
    }
    
    uint32_t* pixels = (uint32_t*)kmalloc(w * h * 4);
    if (!pixels) return NULL;
    
    uint8_t* pixel_data = (uint8_t*)(buffer + header->offset);
    int pitch = ((w * (info->bits / 8)) + 3) & ~3; // 4-byte aligned
    
    for (int y = 0; y < h; y++) {
        int dst_y = bottom_up ? (h - 1 - y) : y;
        uint8_t* row = pixel_data + (y * pitch);
        
        for (int x = 0; x < w; x++) {
            uint32_t color = 0;
            if (info->bits == 24) {
                uint8_t b = row[x*3 + 0];
                uint8_t g = row[x*3 + 1];
                uint8_t r = row[x*3 + 2];
                color = (0xFF << 24) | (r << 16) | (g << 8) | b;
            } else if (info->bits == 32) {
                uint8_t b = row[x*4 + 0];
                uint8_t g = row[x*4 + 1];
                uint8_t r = row[x*4 + 2];
                uint8_t a = row[x*4 + 3];
                color = (a << 24) | (r << 16) | (g << 8) | b;
            }
            pixels[dst_y * w + x] = color;
        }
    }
    
    struct BOSSurface* surface = surface_convert_rgba(pixels, w, h);
    kfree(pixels);
    
    return surface;
}
