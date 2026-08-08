#ifndef ABDE_FONT_H
#define ABDE_FONT_H

#include <stdint.h>

#define ABDE_FONT_WIDTH  8
#define ABDE_FONT_HEIGHT 16

/* Standard 8x16 ASCII Bitmap Font Array (ASCII 32 to 126) */
extern const uint8_t abde_font_8x16[95][16];

#endif // ABDE_FONT_H
