#include "kernel/graphics/display/validation/include/dve_framebuffer.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_framebuffer(const dve_framebuffer_info_t* info, dve_framebuffer_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_framebuffer_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    /* 1. Base Address Check */
    if (info->phys_base == 0) {
        out_result->base_address_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Physical Framebuffer Base Address is NULL (0x0)", sizeof(out_result->failure_reason));
    } else {
        out_result->base_address_valid = true;
    }

    /* 2. Virtual Mapping Check */
    if (!info->virt_base) {
        out_result->virtual_mapped = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Virtual Framebuffer Mapping Pointer is NULL", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->virtual_mapped = true;
    }

    /* 3. Physical Mapping Check */
    out_result->physical_mapped = info->is_phys_mapped;

    /* 4. Memory Alignment Check (64-byte alignment minimum for SIMD blits) */
    if (info->phys_base % 64 != 0 || ((uintptr_t)info->virt_base) % 64 != 0) {
        out_result->memory_aligned = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Framebuffer Address Not 64-Byte Aligned", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->memory_aligned = true;
    }

    /* 5. Page Alignment Check (4KB alignment) */
    if (info->phys_base % 4096 != 0) {
        out_result->page_aligned = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Framebuffer Physical Base Not 4KB Page Aligned", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->page_aligned = true;
    }

    /* 6. Size Sufficiency & Overflow Check */
    if (info->total_size_bytes < (1024 * 768 * 4)) {
        out_result->size_sufficient = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Framebuffer VRAM Size Insufficient (<3MB)", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->size_sufficient = true;
    }

    out_result->overflow_pass = !info->overflow_detected;
    out_result->bounds_check_pass = info->bounds_valid;

    if (!out_result->overflow_pass || !out_result->bounds_check_pass) {
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Framebuffer Memory Bounds Overflow Detected", sizeof(out_result->failure_reason));
        }
    }

    return out_result->status;
}
