#include "kernel/graphics/display/validation/include/dve_pixel_format.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_pixel_format(const dve_pixel_format_info_t* info, dve_pixel_format_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_pixel_format_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    /* 1. Format Supported Check */
    switch (info->format) {
        case DVE_PIXEL_FORMAT_ARGB8888:
        case DVE_PIXEL_FORMAT_XRGB8888:
        case DVE_PIXEL_FORMAT_RGB565:
        case DVE_PIXEL_FORMAT_RGB888:
        case DVE_PIXEL_FORMAT_BGRA8888:
            out_result->format_valid = true;
            break;
        default:
            out_result->format_valid = false;
            out_result->status = DVE_STATUS_FAIL;
            str_copy_safe(out_result->failure_reason, "Unsupported Pixel Format Enumeration", sizeof(out_result->failure_reason));
            return DVE_STATUS_FAIL;
    }

    /* 2. Channel Ordering & Bitmasks Check */
    if (info->format == DVE_PIXEL_FORMAT_ARGB8888 || info->format == DVE_PIXEL_FORMAT_XRGB8888) {
        if (info->red_mask != 0x00FF0000 || info->green_mask != 0x0000FF00 || info->blue_mask != 0x000000FF) {
            out_result->channel_ordering_valid = false;
            out_result->status = DVE_STATUS_FAIL;
            str_copy_safe(out_result->failure_reason, "Channel Ordering Mismatch: ARGB8888 Bitmask Mismatch", sizeof(out_result->failure_reason));
        } else {
            out_result->channel_ordering_valid = true;
        }
    } else if (info->format == DVE_PIXEL_FORMAT_BGRA8888) {
        if (info->blue_mask != 0xFF000000 || info->green_mask != 0x00FF0000 || info->red_mask != 0x0000FF00) {
            out_result->channel_ordering_valid = false;
            out_result->status = DVE_STATUS_FAIL;
            str_copy_safe(out_result->failure_reason, "Channel Ordering Mismatch: BGRA8888 Bitmask Mismatch", sizeof(out_result->failure_reason));
        } else {
            out_result->channel_ordering_valid = true;
        }
    } else {
        out_result->channel_ordering_valid = true;
    }

    /* 3. Alpha Channel Validation */
    if (info->format == DVE_PIXEL_FORMAT_ARGB8888 || info->format == DVE_PIXEL_FORMAT_BGRA8888) {
        if (!info->has_alpha || info->alpha_mask == 0) {
            out_result->alpha_valid = false;
            out_result->status = DVE_STATUS_FAIL;
            if (out_result->failure_reason[0] == '\0') {
                str_copy_safe(out_result->failure_reason, "Alpha Mismatch: Format requires alpha channel but alpha_mask is 0", sizeof(out_result->failure_reason));
            }
        } else {
            out_result->alpha_valid = true;
        }
    } else {
        out_result->alpha_valid = true;
    }

    return out_result->status;
}
