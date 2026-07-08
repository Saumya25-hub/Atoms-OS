#include "pointer_precision.h"
#include "kernel/drivers/display/display.h"

void pointer_precision_init(void) {
    display_print("[POINTER PRECISION] 16.16 Fixed-Point Sub-Pixel Accumulator Initialized.\n");
}

void pointer_precision_accumulate(int32_t dx_int, int32_t dy_int, int32_t accel_fp16,
                                  int32_t* inout_sub_x, int32_t* inout_sub_y,
                                  int32_t* out_whole_dx, int32_t* out_whole_dy) {
    if (!inout_sub_x || !inout_sub_y || !out_whole_dx || !out_whole_dy) {
        return;
    }

    // Convert raw integer movement to 16.16 fixed-point and apply ballistic acceleration multiplier
    int64_t scaled_dx = ((int64_t)FP16_FROM_INT(dx_int) * (int64_t)accel_fp16) >> 16;
    int64_t scaled_dy = ((int64_t)FP16_FROM_INT(dy_int) * (int64_t)accel_fp16) >> 16;

    // Record previous whole pixel position
    int32_t old_whole_x = FP16_TO_INT(*inout_sub_x);
    int32_t old_whole_y = FP16_TO_INT(*inout_sub_y);

    // Accumulate sub-pixel movement
    *inout_sub_x += (int32_t)scaled_dx;
    *inout_sub_y += (int32_t)scaled_dy;

    // Extract new whole pixel position
    int32_t new_whole_x = FP16_TO_INT(*inout_sub_x);
    int32_t new_whole_y = FP16_TO_INT(*inout_sub_y);

    // Compute whole pixel delta for display rendering
    *out_whole_dx = new_whole_x - old_whole_x;
    *out_whole_dy = new_whole_y - old_whole_y;
}
