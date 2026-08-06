#include "kernel/graphics/display/validation/include/dve_geometry.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_geometry(const dve_geometry_info_t* info, dve_geometry_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_geometry_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    uint32_t bpp_bytes = (info->bpp > 0) ? (info->bpp / 8) : 4;
    if (bpp_bytes == 0) bpp_bytes = 4;

    out_result->expected_width = info->width;
    out_result->actual_width = info->width;
    out_result->expected_height = info->height;
    out_result->actual_height = info->height;

    out_result->expected_pitch = info->width * bpp_bytes;
    out_result->actual_pitch = info->pitch;

    out_result->expected_frame_size = (uint64_t)out_result->expected_pitch * info->height;
    out_result->actual_frame_size = info->actual_frame_size ? info->actual_frame_size : ((uint64_t)info->pitch * info->height);

    /* 1. Pitch Match Check */
    if (info->pitch < out_result->expected_pitch) {
        out_result->pitch_match = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Pitch Mismatch: Actual Pitch is Smaller than Width * BPP", sizeof(out_result->failure_reason));
    } else if (info->pitch != out_result->expected_pitch && info->surface_pitch > 0 && info->pitch != info->surface_pitch) {
        out_result->pitch_match = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Pitch Mismatch: Framebuffer Pitch Conflict with Surface Pitch", sizeof(out_result->failure_reason));
    } else {
        out_result->pitch_match = true;
    }

    /* 2. Stride Match Check */
    uint32_t calc_stride = info->pitch / bpp_bytes;
    if (info->stride > 0 && info->stride != calc_stride) {
        out_result->stride_match = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Stride Mismatch: Stride * BPP does not match Framebuffer Pitch", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->stride_match = true;
    }

    /* 3. Resolution & Dimension Limits Check */
    if (info->width < 640 || info->width > 7680 || info->height < 480 || info->height > 4320) {
        out_result->resolution_match = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Resolution Mismatch: Dimensions Outside 640x480..7680x4320 Boundary", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->resolution_match = true;
    }

    return out_result->status;
}
