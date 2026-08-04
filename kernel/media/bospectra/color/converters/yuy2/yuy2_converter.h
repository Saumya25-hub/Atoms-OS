#ifndef YUY2_CONVERTER_H
#define YUY2_CONVERTER_H

#include "../../include/bospectra_color.h"

bospectra_error_t bospectra_convert_yuy2_to_argb32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space);

#endif // YUY2_CONVERTER_H
