/**
 * @file bos_cursor_hal.h
 * @brief GPU Hardware Overlay Upload HAL Bridge for BCE V1.0
 */

#ifndef BOS_CURSOR_HAL_H
#define BOS_CURSOR_HAL_H

#include "bos_cursor.h"

#ifdef __cplusplus
extern "C" {
#endif

bce_error_t bos_cursor_hal_init(void);
bce_error_t bos_cursor_hal_upload_frame(const bce_frame_t* frame);
bce_error_t bos_cursor_hal_set_position(int32_t x, int32_t y);
bce_error_t bos_cursor_hal_set_visibility(bool visible);
bool        bos_cursor_hal_is_hardware_active(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CURSOR_HAL_H */
