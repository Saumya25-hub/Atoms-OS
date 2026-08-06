#include "kernel/graphics/display/validation/include/dve_gpu_driver.h"

static void str_copy_safe(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (src && src[i] != '\0' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

dve_status_t dve_validate_gpu_driver(const dve_gpu_driver_info_t* info, dve_gpu_driver_result_t* out_result) {
    if (!info || !out_result) return DVE_STATUS_FAIL;

    for (size_t i = 0; i < sizeof(dve_gpu_driver_result_t); i++) {
        ((uint8_t*)out_result)[i] = 0;
    }

    out_result->status = DVE_STATUS_PASS;

    /* 1. Driver Loaded Check */
    if (!info->driver_loaded) {
        out_result->loaded_pass = false;
        out_result->status = DVE_STATUS_FAIL;
        str_copy_safe(out_result->failure_reason, "GPU Driver Not Loaded into Subsystem Registry", sizeof(out_result->failure_reason));
    } else {
        out_result->loaded_pass = true;
    }

    /* 2. Driver Selected Check */
    if (!info->driver_selected) {
        out_result->selection_pass = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "GPU Driver Not Selected as Active Backend", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->selection_pass = true;
    }

    /* 3. Vendor & Device ID Verification */
    if (info->vendor_id == 0 || info->vendor_id == 0xFFFF) {
        /* Check if it's Software Renderer fallback */
        if (info->driver_name && (info->driver_name[0] == 'S' || info->driver_name[0] == 's')) {
            out_result->vendor_device_valid = true;
        } else {
            out_result->vendor_device_valid = false;
            out_result->status = DVE_STATUS_FAIL;
            if (out_result->failure_reason[0] == '\0') {
                str_copy_safe(out_result->failure_reason, "PCI Vendor ID Invalid (0x0 or 0xFFFF)", sizeof(out_result->failure_reason));
            }
        }
    } else {
        out_result->vendor_device_valid = true;
    }

    /* 4. BAR MMIO & VRAM Mapping Check */
    if (info->vendor_id != 0xFFFF && (info->bar0_base == 0 || info->bar0_size == 0)) {
        out_result->bar_mapping_valid = false;
        out_result->status = DVE_STATUS_FAIL;
        if (out_result->failure_reason[0] == '\0') {
            str_copy_safe(out_result->failure_reason, "PCI BAR0 MMIO Base/Size Unmapped", sizeof(out_result->failure_reason));
        }
    } else {
        out_result->bar_mapping_valid = true;
    }

    out_result->mmio_mapping_valid = info->mmio_mapped;
    out_result->capabilities_valid = (info->capabilities != 0);

    return out_result->status;
}
