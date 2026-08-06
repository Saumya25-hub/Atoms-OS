/**
 * @file cursor_hal.c
 * @brief GPU Hardware Overlay Upload HAL Bridge Implementation
 */

#include "../include/bos_cursor_hal.h"
#include "../include/bos_cursor_diag.h"
#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/debug/step14_telemetry.h"

static bool g_hw_cursor_active = false;

bce_error_t bos_cursor_hal_init(void) {
    g_hw_cursor_active = false;
    bos_gpu_device_t* gpu = bos_gpu_get_primary();
    if (gpu && gpu->ops && gpu->ops->set_cursor_position && gpu->ops->set_cursor_image) {
        g_hw_cursor_active = true;
    }
    return BCE_OK;
}

bce_error_t bos_cursor_hal_upload_frame(const bce_frame_t* frame) {
    if (!frame || !frame->argb_pixels) return BCE_ERR_INVALID_PARAM;

    uint64_t start_tsc = step14_rdtsc();

    /* Update software presenter bitmap so compositor redraws get the new animated frame */
    extern void BSPE_CursorPresenter_UpdateBitmap(const uint32_t* bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y);
    BSPE_CursorPresenter_UpdateBitmap(frame->argb_pixels, frame->width, frame->height, frame->hotspot_x, frame->hotspot_y);

    bos_gpu_device_t* gpu = bos_gpu_get_primary();
    if (gpu && gpu->ops && gpu->ops->set_cursor_image) {
        bos_gpu_status_t st = gpu->ops->set_cursor_image(gpu, frame->argb_pixels, frame->width, frame->height, frame->hotspot_x, frame->hotspot_y);
        if (st == BOS_GPU_OK) {
            g_hw_cursor_active = true;
            uint64_t end_tsc = step14_rdtsc();
            bos_cursor_diag_log_upload((uint32_t)step14_cycles_to_us(end_tsc - start_tsc));
            return BCE_OK;
        }
    }

    g_hw_cursor_active = false;
    return BCE_ERR_GPU_FAIL;
}

bce_error_t bos_cursor_hal_set_position(int32_t x, int32_t y) {
    bos_gpu_device_t* gpu = bos_gpu_get_primary();
    if (gpu && gpu->ops && gpu->ops->set_cursor_position) {
        if (gpu->ops->set_cursor_position(gpu, x, y) == BOS_GPU_OK) {
            return BCE_OK;
        }
    }
    return BCE_ERR_GPU_FAIL;
}

bce_error_t bos_cursor_hal_set_visibility(bool visible) {
    bos_gpu_device_t* gpu = bos_gpu_get_primary();
    if (gpu && gpu->ops && gpu->ops->set_cursor_visibility) {
        if (gpu->ops->set_cursor_visibility(gpu, visible) == BOS_GPU_OK) {
            return BCE_OK;
        }
    }
    return BCE_ERR_GPU_FAIL;
}

bool bos_cursor_hal_is_hardware_active(void) {
    return g_hw_cursor_active;
}
