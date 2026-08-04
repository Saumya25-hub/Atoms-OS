#ifndef RGB_CONVERTER_H
#define RGB_CONVERTER_H

#include "../../include/bospectra_color.h"

bospectra_error_t bospectra_convert_rgb24_to_argb32(const BOSFrame* src, BOSFrame* dst);
bospectra_error_t bospectra_swizzle_argb32_to_rgba32(const BOSFrame* src, BOSFrame* dst);

#endif // RGB_CONVERTER_H
