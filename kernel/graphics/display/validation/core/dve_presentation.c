#include "kernel/graphics/display/validation/include/dve_presentation.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_presentation(const dve_presentation_info_t* info, dve_presentation_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_presentation_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    out_result->expected_src_pitch = info->src_width * 4;
    out_result->actual_src_pitch = info->src_pitch;

    out_result->expected_dst_pitch = info->dst_width * 4;
    out_result->actual_dst_pitch = info->dst_pitch;

    out_result->expected_copy_size = (uint64_t)out_result->expected_src_pitch * info->src_height;
    out_result->actual_copy_size = info->copy_size_bytes ? info->copy_size_bytes : ((uint64_t)info->dst_pitch * info->dst_height);

    /* 1. Pitch Match Check */
    if (info->src_pitch != info->dst_pitch) {
        out_result->pitch_match_pass = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Presentation Pitch Mismatch: Source Pitch != Destination Pitch", sizeof(out_result->failure_reason));
    } else {
        out_result->pitch_match_pass = true;
    }

    /* 2. Copy Size Verification */
    if (out_result->actual_copy_size < out_result->expected_copy_size) {
        out_result->copy_size_pass = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Copy Size Mismatch: Actual Transfer Size < Expected Frame Size", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->copy_size_pass = true;
    }

    /* 3. Hardware Present & Page Flip Check */
    out_result->present_call_pass = true;
    out_result->page_flip_pass = info->page_flip_supported;
    out_result->buffer_swap_pass = info->buffer_swap_active;
    out_result->dma_copy_pass = info->dma_copy_active;
    out_result->hw_present_pass = info->hw_present_active;

    if (!out_result->hw_present_pass) {
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Hardware Present Engine Inactive or Flushed Early", sizeof(out_result->failure_reason));
        }
    }

    return out_result->status;
}
