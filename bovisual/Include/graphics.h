#ifndef BOVISUAL_GRAPHICS_H
#define BOVISUAL_GRAPHICS_H

#include "bovisual_types.h"

// Note: These functions are intended for use by internal Drawing/Controls modules.

// Set a single pixel in the framebuffer to a specific color
void BOVISUAL_Graphics_PutPixel(int32_t x, int32_t y, BOVISUAL_Color color);

// Get the color of a specific pixel in the framebuffer
BOVISUAL_Color BOVISUAL_Graphics_ReadPixel(int32_t x, int32_t y);

// Clear the entire framebuffer to a specific color
void BOVISUAL_Graphics_Clear(BOVISUAL_Color color);

// Fill a specific absolute rectangle area with a color
void BOVISUAL_Graphics_Fill(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color);

#endif // BOVISUAL_GRAPHICS_H
