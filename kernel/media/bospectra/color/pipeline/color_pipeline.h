#ifndef COLOR_PIPELINE_H
#define COLOR_PIPELINE_H

#include "../include/bospectra_color.h"

bospectra_error_t bospectra_color_pipeline_process(const BOSFrame* src_frame, bospectra_pixel_format_t target_format, bospectra_color_space_t color_space, BOSFrame** out_converted_frame);

#endif // COLOR_PIPELINE_H
