#include "color_pipeline.h"
#include "../converters/yuv420/yuv420_converter.h"
#include "../converters/nv12/nv12_converter.h"
#include "../converters/yuy2/yuy2_converter.h"
#include "../converters/rgb/rgb_converter.h"
#include "../../frame_memory/frame_pool/frame_pool.h"
#include "../../include/bospectra_errors.h"

bospectra_error_t bospectra_color_pipeline_process(const BOSFrame* src_frame, bospectra_pixel_format_t target_format, bospectra_color_space_t color_space, BOSFrame** out_converted_frame) {
    if (!src_frame || !out_converted_frame) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }
    if (!bospectra_frame_is_valid(src_frame)) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    if (target_format == BOSPECTRA_PIXEL_FORMAT_UNKNOWN) {
        target_format = BOSPECTRA_PIXEL_FORMAT_ARGB32; // Default display surface format
    }

    // Direct Pass-Through if source format matches target format
    if (src_frame->format == target_format) {
        bospectra_frame_ref((BOSFrame*)src_frame);
        *out_converted_frame = (BOSFrame*)src_frame;
        return BOSPECTRA_SUCCESS;
    }

    // Acquire pre-allocated destination frame from Phase 3 Frame Pool (zero-heap allocation during playback)
    BOSFrame* dst_frame = NULL;
    bospectra_error_t err = bospectra_frame_acquire(src_frame->width, src_frame->height, target_format, &dst_frame);
    if (err != BOSPECTRA_SUCCESS || !dst_frame) {
        return BOSPECTRA_ERR_OUT_OF_MEMORY;
    }

    // Dispatch to dedicated converter driver based on source format
    switch (src_frame->format) {
        case BOSPECTRA_PIXEL_FORMAT_YUV420P:
            if (target_format == BOSPECTRA_PIXEL_FORMAT_ARGB32) {
                err = bospectra_convert_yuv420p_to_argb32(src_frame, dst_frame, color_space);
            } else if (target_format == BOSPECTRA_PIXEL_FORMAT_RGBA32) {
                err = bospectra_convert_yuv420p_to_rgba32(src_frame, dst_frame, color_space);
            } else if (target_format == BOSPECTRA_PIXEL_FORMAT_RGB24) {
                err = bospectra_convert_yuv420p_to_rgb24(src_frame, dst_frame, color_space);
            } else {
                err = BOSPECTRA_ERR_UNSUPPORTED_FORMAT;
            }
            break;

        case BOSPECTRA_PIXEL_FORMAT_NV12:
            if (target_format == BOSPECTRA_PIXEL_FORMAT_ARGB32) {
                err = bospectra_convert_nv12_to_argb32(src_frame, dst_frame, color_space);
            } else if (target_format == BOSPECTRA_PIXEL_FORMAT_RGBA32) {
                err = bospectra_convert_nv12_to_rgba32(src_frame, dst_frame, color_space);
            } else {
                err = BOSPECTRA_ERR_UNSUPPORTED_FORMAT;
            }
            break;

        case BOSPECTRA_PIXEL_FORMAT_YUY2:
            if (target_format == BOSPECTRA_PIXEL_FORMAT_ARGB32) {
                err = bospectra_convert_yuy2_to_argb32(src_frame, dst_frame, color_space);
            } else {
                err = BOSPECTRA_ERR_UNSUPPORTED_FORMAT;
            }
            break;

        case BOSPECTRA_PIXEL_FORMAT_RGB24:
            if (target_format == BOSPECTRA_PIXEL_FORMAT_ARGB32) {
                err = bospectra_convert_rgb24_to_argb32(src_frame, dst_frame);
            } else {
                err = BOSPECTRA_ERR_UNSUPPORTED_FORMAT;
            }
            break;

        default:
            err = BOSPECTRA_ERR_UNSUPPORTED_FORMAT;
            break;
    }

    if (err != BOSPECTRA_SUCCESS) {
        bospectra_frame_release(dst_frame);
        *out_converted_frame = NULL;
        return err;
    }

    *out_converted_frame = dst_frame;
    return BOSPECTRA_SUCCESS;
}
