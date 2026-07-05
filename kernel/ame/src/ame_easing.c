#include "../include/ame.h"

// 16.16 Fixed-Point Easing Mathematics Engine
// 1.0 = 65536 (0x10000)
// Evaluates parametric curves without floating-point FPU/SSE overhead in kernel space.

static const int32_t s_elastic_lut[9] = {
    0,      // 0.000 -> 0.000
    88836,  // 0.125 -> 1.355 (overshoot)
    49152,  // 0.250 -> 0.750 (undershoot)
    70449,  // 0.375 -> 1.075 (overshoot)
    63570,  // 0.500 -> 0.970
    66322,  // 0.625 -> 1.012
    65208,  // 0.750 -> 0.995
    65667,  // 0.875 -> 1.002
    65536   // 1.000 -> 1.000
};

int32_t AME_EvaluateCurve(AME_EasingCurve curve, int32_t progress_fixed16) {
    if (progress_fixed16 <= 0) return 0;
    if (progress_fixed16 >= 65536) return 65536;

    switch (curve) {
        case EASE_LINEAR:
            return progress_fixed16;

        case EASE_OUT_CUBIC: {
            // f(t) = 1 - (1 - t)^3 (64-bit precision without intermediate truncation)
            int64_t q = 65536 - progress_fixed16;
            int64_t q3 = q * q * q;
            return (int32_t)(65536 - (q3 >> 32));
        }

        case EASE_IN_OUT_CUBIC: {
            // Piecewise cubic Hermite interpolation (64-bit precision without intermediate truncation)
            if (progress_fixed16 < 32768) {
                int64_t p = progress_fixed16;
                int64_t p3 = p * p * p;
                return (int32_t)((4 * p3) >> 32);
            } else {
                int64_t q = 131072 - (2 * progress_fixed16);
                int64_t q3 = q * q * q;
                return (int32_t)(65536 - ((q3 >> 32) >> 1));
            }
        }

        case EASE_OUT_QUINT: {
            // f(t) = 1 - (1 - t)^5
            int64_t q = 65536 - progress_fixed16;
            int64_t q2 = (q * q) >> 16;
            int64_t q4 = (q2 * q2) >> 16;
            int64_t q5 = (q4 * q) >> 16;
            return (int32_t)(65536 - q5);
        }

        case EASE_OUT_BOUNCE: {
            // Exact piecewise quadratic polynomial evaluation in fixed point
            int64_t p = progress_fixed16;
            if (p < 23795) { // < 1 / 2.75
                return (int32_t)((495616LL * ((p * p) >> 16)) >> 16);
            } else if (p < 47590) { // < 2 / 2.75
                p -= 35692;
                return (int32_t)(((495616LL * ((p * p) >> 16)) >> 16) + 49152);
            } else if (p < 59487) { // < 2.5 / 2.75
                p -= 53538;
                return (int32_t)(((495616LL * ((p * p) >> 16)) >> 16) + 60621);
            } else {
                p -= 62462;
                return (int32_t)(((495616LL * ((p * p) >> 16)) >> 16) + 64512);
            }
        }

        case EASE_OUT_ELASTIC: {
            // High-speed LUT evaluation with linear interpolation
            int32_t idx = (progress_fixed16 * 8) >> 16;
            if (idx >= 8) return 65536;
            int32_t frac = (progress_fixed16 * 8) & 0xFFFF;
            int32_t val0 = s_elastic_lut[idx];
            int32_t val1 = s_elastic_lut[idx + 1];
            return val0 + (((val1 - val0) * frac) >> 16);
        }

        default:
            return progress_fixed16;
    }
}
