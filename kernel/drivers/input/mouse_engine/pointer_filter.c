#include "pointer_filter.h"
#include "pointer_diag.h"
#include "kernel/drivers/input/pointer/pointer_precision.h"
#include "kernel/drivers/input/pointer/pointer_velocity.h"
#include "kernel/drivers/input/pointer/pointer_predict.h"

#include "kernel/debug/step14_telemetry.h"

static int32_t s_sub_x = 0;
static int32_t s_sub_y = 0;
static bool s_precision_initialized = false;

void pointer_filter_init(void) {
    pointer_precision_init();
    pointer_velocity_init();
    pointer_predict_init();
    s_sub_x = 0;
    s_sub_y = 0;
    s_precision_initialized = true;
}

void pointer_filter_apply(int32_t raw_dx, int32_t raw_dy, int32_t* out_dx, int32_t* out_dy) {
    if (!out_dx || !out_dy) return;

    if (!s_precision_initialized) {
        pointer_filter_init();
    }

    uint64_t now_us = step14_cycles_to_us(step14_rdtsc());
    int32_t raw_vel = 0;
    int32_t accel_fp16 = pointer_velocity_calculate(raw_dx, raw_dy, now_us, &raw_vel);

    if (accel_fp16 <= 0) {
        accel_fp16 = FP16_ONE; /* 1.0x fallback */
    }

    /* Apply 16.16 Fixed-Point Sub-Pixel Accumulator (Glider Engine) */
    int32_t whole_dx = 0, whole_dy = 0;
    pointer_precision_accumulate(raw_dx, raw_dy, accel_fp16, &s_sub_x, &s_sub_y, &whole_dx, &whole_dy);

    /* Windows NT Kinematic Trajectory Subpixel Prediction (Floating-on-Glass V3.0) */
    int32_t pred_dx = whole_dx, pred_dy = whole_dy;
    pointer_predict_apply(whole_dx, whole_dy, now_us, 8333U, &pred_dx, &pred_dy);

    *out_dx = pred_dx;
    *out_dy = pred_dy;

    pointer_diag_update_filtered(*out_dx, *out_dy);
}
