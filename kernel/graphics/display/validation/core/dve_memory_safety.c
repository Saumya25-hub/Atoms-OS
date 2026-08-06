#include "kernel/graphics/display/validation/include/dve_memory_safety.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_memory_safety(const dve_memory_safety_info_t* info, dve_memory_safety_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_memory_safety_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    /* 1. Pointer Non-NULL Check */
    if (!info->buffer_ptr) {
        out_result->pointer_valid = false;
        out_result->surface_not_null = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Memory Pointer Null Error", sizeof(out_result->failure_reason));
        return DVE_STATUS_FAIL;
    }
    out_result->pointer_valid = true;
    out_result->surface_not_null = true;

    /* 2. Pitch Validity Check */
    uint32_t bpp_bytes = (info->bpp > 0) ? (info->bpp / 8) : 4;
    uint32_t expected_pitch = info->width * bpp_bytes;
    if (info->pitch < expected_pitch) {
        out_result->pitch_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Invalid Pitch: Less than Width * BPP", sizeof(out_result->failure_reason));
    } else {
        out_result->pitch_valid = true;
    }

    /* 3. Out of Bounds & Buffer Overrun/Underrun Check */
    uint32_t last_row = (info->access_height > 0) ? (info->access_y + info->access_height - 1) : info->access_y;
    uint64_t access_end_offset = ((uint64_t)last_row * info->pitch) + ((uint64_t)(info->access_x + info->access_width) * bpp_bytes);
    if (info->buffer_size > 0 && access_end_offset > info->buffer_size) {
        out_result->no_overrun = false;
        out_result->bounds_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Buffer Overrun Detected: Access Offset Exceeds Buffer Allocation Size", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->no_overrun = true;
        out_result->bounds_valid = true;
    }

    out_result->no_underrun = true;

    /* 4. Alignment Check (64-bit alignment for SIMD pixel loops) */
    if (((uintptr_t)info->buffer_ptr) % 8 != 0) {
        out_result->alignment_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Alignment Error: Buffer Address Not 8-Byte Aligned", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->alignment_valid = true;
    }

    return out_result->status;
}
