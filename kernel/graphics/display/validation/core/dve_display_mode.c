#include "kernel/graphics/display/validation/include/dve_display_mode.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_display_mode(const dve_display_mode_info_t* info, dve_display_mode_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_display_mode_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    /* 1. Resolution Boundaries Check */
    if (info->width < 640 || info->width > 7680 || info->height < 480 || info->height > 4320) {
        out_result->resolution_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "Display Resolution Invalid (<640x480 or >7680x4320)", sizeof(out_result->failure_reason));
    } else {
        out_result->resolution_valid = true;
    }

    /* 2. Refresh Rate Check (24Hz..240Hz) */
    if (info->refresh_rate < 24 || info->refresh_rate > 240) {
        out_result->refresh_rate_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Refresh Rate Out of Boundary (24Hz..240Hz)", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->refresh_rate_valid = true;
    }

    /* 3. Pixel Clock & Sync Timings Check */
    if (info->pixel_clock_khz == 0 || info->htotal < info->width || info->vtotal < info->height) {
        out_result->timing_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Display Sync Timings / Pixel Clock Invalid", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->timing_valid = true;
    }

    /* 4. Scanout & Monitor State Check */
    out_result->scanout_valid = info->scanout_active;
    out_result->monitor_connected = info->monitor_connected;
    out_result->mode_supported = info->edid_valid;

    if (!out_result->scanout_valid || !out_result->monitor_connected) {
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "Scanout Engine Inactive or Monitor Disconnected", sizeof(out_result->failure_reason));
        }
    }

    return out_result->status;
}
