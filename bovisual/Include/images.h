#ifndef BOVISUAL_IMAGES_H
#define BOVISUAL_IMAGES_H

#include "bovisual_types.h"

// Note: Internal engine API for drawing BMP images.

// Draw a 24-bit or 32-bit uncompressed BMP from a memory buffer
// Returns false if the format is unsupported or invalid.
bool BOVISUAL_Draw_BMP(int32_t x, int32_t y, const uint8_t* bmp_data, uint32_t data_size);

#endif // BOVISUAL_IMAGES_H
