#include "kernel/graphics/display/validation/include/dve_surface.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_surface(const dve_surface_info_t* info, dve_surface_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_surface_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    /* 1. Surface Dimensions Check */
    if (info->width == 0 || info->height == 0 || info->width > 7680 || info->height > 4320) {
        out_result->dimensions_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Surface Dimensions Invalid (0 or >7680x4320)", sizeof(out_result->failure_reason));
    } else {
        out_result->dimensions_valid = true;
    }

    /* 2. Surface Pitch Check */
    if (info->pitch < info->width * 4) {
        out_result->pitch_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Surface Pitch Invalid (< Width * 4)", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->pitch_valid = true;
    }

    /* 3. Surface Pixel Format Check */
    if (info->format == DVE_PIXEL_FORMAT_UNKNOWN) {
        out_result->format_supported = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Surface Pixel Format Unknown", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->format_supported = true;
    }

    /* 4. Surface Memory Alignment Check */
    if (info->alignment > 0 && info->pitch % info->alignment != 0) {
        out_result->alignment_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Surface Pitch Alignment Violation", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->alignment_valid = true;
    }

    /* 5. Surface Lifetime Tracking Check */
    if (!info->handle_valid || !info->is_active) {
        out_result->lifetime_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Surface Handle Invalid or Orphaned Lifetime", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->lifetime_valid = true;
    }

    return out_result->status;
}
