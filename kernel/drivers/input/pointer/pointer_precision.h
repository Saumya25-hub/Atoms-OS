#ifndef KERNEL_POINTER_PRECISION_H
#define KERNEL_POINTER_PRECISION_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: 16.16 Fixed-Point Sub-Pixel Math
// ============================================================================
// Preserves fractional motion from high-DPI mice, touchpads, and drawing tablets
// without integer truncation or floating-point FPU overhead in kernel space.
// ============================================================================

#define FP16_ONE (1 << 16)
#define FP16_FROM_INT(x) ((x) << 16)
#define FP16_TO_INT(x) ((x) >> 16)

// Initialize precision math module
void pointer_precision_init(void);

// Accumulate scaled movement into 16.16 fixed-point sub-pixel counters and extract whole pixels
void pointer_precision_accumulate(int32_t dx_int, int32_t dy_int, int32_t accel_fp16,
                                  int32_t* inout_sub_x, int32_t* inout_sub_y,
                                  int32_t* out_whole_dx, int32_t* out_whole_dy);

#endif // KERNEL_POINTER_PRECISION_H
