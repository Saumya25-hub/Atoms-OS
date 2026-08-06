#include "kernel/graphics/display/validation/include/dve_frame_integrity.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* Fast hardware-style CRC32 calculation over framebuffer bytes */
static uint32_t calculate_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int k = 0; k < 8; k++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = (crc >> 1);
            }
        }
    }
    return ~crc;
}

/* Fast 64-bit FNV-1a hash over frame buffer */
static uint64_t calculate_fnv1a_hash(const uint8_t* data, size_t length) {
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < length; i += 4) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

dve_status_t dve_validate_frame_integrity(const dve_frame_integrity_info_t* info, dve_frame_integrity_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_frame_integrity_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    if (!info->buffer_ptr || info->width == 0 || info->height == 0) {
        out_result->status = DVE_STATUS_FAIL;
        out_result->is_dead_frame = true;
        str_copy_safe(out_result->failure_reason, "Frame Integrity FAILED: Null Framebuffer Pointer or 0 Dimensions", sizeof(out_result->failure_reason));
        return DVE_STATUS_FAIL;
    }

    const uint32_t* pixels = (const uint32_t*)info->buffer_ptr;
    uint32_t bpp_bytes = (info->bpp > 0) ? (info->bpp / 8) : 4;
    uint32_t pitch_pixels = info->pitch / bpp_bytes;
    if (pitch_pixels == 0) pitch_pixels = info->width;

    out_result->total_pixels = (uint64_t)info->width * info->height;
    size_t bytes_to_sample = out_result->total_pixels * bpp_bytes;

    out_result->crc32 = calculate_crc32((const uint8_t*)info->buffer_ptr, bytes_to_sample > 65536 ? 65536 : bytes_to_sample);
    out_result->fnv1a_hash = calculate_fnv1a_hash((const uint8_t*)info->buffer_ptr, bytes_to_sample > 65536 ? 65536 : bytes_to_sample);

    uint64_t black_count = 0;
    uint64_t visible_count = 0;
    uint64_t r_sum = 0, g_sum = 0, b_sum = 0, a_sum = 0;

    for (uint32_t y = 0; y < info->height; y++) {
        const uint32_t* row = pixels + (y * pitch_pixels);
        for (uint32_t x = 0; x < info->width; x++) {
            uint32_t px = row[x];
            uint8_t a = (px >> 24) & 0xFF;
            uint8_t r = (px >> 16) & 0xFF;
            uint8_t g = (px >> 8) & 0xFF;
            uint8_t b = px & 0xFF;

            r_sum += r; g_sum += g; b_sum += b; a_sum += a;

            /* Check if pixel is pure black 0x00000000 or 0xFF000000 */
            if ((px & 0x00FFFFFF) == 0) {
                black_count++;
            } else {
                visible_count++;
            }
        }
    }

    out_result->black_pixels = black_count;
    out_result->visible_pixels = visible_count;
    out_result->sum_red = r_sum;
    out_result->sum_green = g_sum;
    out_result->sum_blue = b_sum;
    out_result->sum_alpha = a_sum;

    if (out_result->total_pixels > 0) {
        out_result->black_pixel_percentage = ((float)black_count / (float)out_result->total_pixels) * 100.0f;
        out_result->visible_pixel_percentage = ((float)visible_count / (float)out_result->total_pixels) * 100.0f;
    }

    out_result->entropy_score = (uint32_t)(visible_count * 100 / (out_result->total_pixels ? out_result->total_pixels : 1));

    /* DEAD FRAME DETECTION:
     * If black_pixel_percentage > 99.0% or visible_pixel_percentage < 0.1%,
     * flag as FAILED DEAD FRAME!
     */
    if (out_result->black_pixel_percentage > 99.0f || out_result->visible_pixel_percentage < 0.1f) {
        out_result->is_dead_frame = true;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Frame Integrity FAILED: Rendered Image is Empty / Dead Frame (>99% Black Pixels)", sizeof(out_result->failure_reason));
    } else {
        out_result->is_dead_frame = false;
    }

    return out_result->status;
}
