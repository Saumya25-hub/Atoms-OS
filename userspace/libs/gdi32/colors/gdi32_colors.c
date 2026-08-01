#include "../include/gdi32_api.h"

COLORREF gdi32_color_create_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return RGB(r, g, b) | ((uint32_t)a << 24);
}
