#ifndef YUV420_CONVERTER_H
#define YUV420_CONVERTER_H

#include "../../include/bospectra_color.h"

bospectra_error_t bospectra_convert_yuv420p_to_argb32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space);
bospectra_error_t bospectra_convert_yuv420p_to_rgba32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space);
bospectra_error_t bospectra_convert_yuv420p_to_rgb24(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space);

#endif // YUV420_CONVERTER_H
