#ifndef BOPAWN_IMAGE_FORMAT_H
#define BOPAWN_IMAGE_FORMAT_H

#include "kernel/gui/surface/surface.h"
#include <stdint.h>

// Internal image formats
typedef enum {
    BOPAWN_FORMAT_UNKNOWN = 0,
    BOPAWN_FORMAT_PNG,
    BOPAWN_FORMAT_BMP,
    BOPAWN_FORMAT_RAW,
    BOPAWN_FORMAT_ICO
} BOPAWN_Format;

// Internal Image Structure
typedef struct BOSImage {
    BOPAWN_Format format;
    struct BOSSurface* surface; // Resulting rendered pixels
    int width;
    int height;
    int ref_count;              // For cache tracking
    char filepath[256];
} BOSImage;

// Decoder function signatures
typedef struct BOSSurface* (*BopawnDecoderFunc)(const uint8_t* buffer, uint32_t size);

struct BOSSurface* png_decode(const uint8_t* buffer, uint32_t size);
struct BOSSurface* bmp_decode(const uint8_t* buffer, uint32_t size);
struct BOSSurface* raw_decode(const uint8_t* buffer, uint32_t size);
struct BOSSurface* ico_decode(const uint8_t* buffer, uint32_t size);

// Format detection based on magic bytes
BOPAWN_Format bopawn_detect_format(const uint8_t* buffer, uint32_t size);

// Conversion utility to wrap raw pixels in BOSSurface
struct BOSSurface* surface_convert_rgba(uint32_t* pixels, int width, int height);

#endif // BOPAWN_IMAGE_FORMAT_H
