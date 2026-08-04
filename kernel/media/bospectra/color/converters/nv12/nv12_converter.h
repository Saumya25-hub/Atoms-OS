#ifndef NV12_CONVERTER_H
#define NV12_CONVERTER_H

#include "../../include/bospectra_color.h"

bospectra_error_t bospectra_convert_nv12_to_argb32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space);
bospectra_error_t bospectra_convert_nv12_to_rgba32(const BOSFrame* src, BOSFrame* dst, bospectra_color_space_t color_space);

#endif // NV12_CONVERTER_H
