#ifndef BOVISUAL_TYPES_H
#define BOVISUAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "bovisual_config.h"

// Colors are ARGB 32-bit (0xAARRGGBB) for standard true color
typedef uint32_t BOVISUAL_Color;

// Primitive Point
typedef struct {
    int32_t x;
    int32_t y;
} BVPoint;

// Primitive Size
typedef struct {
    int32_t width;
    int32_t height;
} BVSize;

// Primitive Rectangle
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} BVRect;

// Primitive Padding
typedef struct {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} BVPadding;

// Framebuffer Description
typedef struct {
    BOVISUAL_Color* buffer; // Pointer to start of pixel data
    uint32_t width;
    uint32_t height;
    uint32_t pitch;         // bytes per row
} BVFramebuffer;

#endif // BOVISUAL_TYPES_H
