#include "../include/ame.h"

/*
 * 🎬 ATOMS Motion Engine (AME) — Helper Animation Modules
 * Renderer-independent parametric animation calculators for Fade, Scale, Position, and Interpolation.
 * Zero heap allocation, 16.16 fixed-point precision.
 */

int32_t AME_Interpolate(int32_t start, int32_t end, int32_t progress_fixed16) {
    if (progress_fixed16 <= 0) return start;
    if (progress_fixed16 >= 65536) return end;
    int64_t diff = (int64_t)end - (int64_t)start;
    return start + (int32_t)((diff * progress_fixed16) >> 16);
}

int32_t AME_Fade_Calculate(uint64_t elapsed_ms, uint32_t duration_ms, uint8_t start_alpha, uint8_t end_alpha, AME_EasingCurve curve) {
    if (duration_ms == 0 || elapsed_ms >= duration_ms) return end_alpha;
    int32_t progress = (int32_t)((elapsed_ms * 65536ULL) / duration_ms);
    int32_t eased = AME_EvaluateCurve(curve, progress);
    return AME_Interpolate((int32_t)start_alpha, (int32_t)end_alpha, eased);
}

int32_t AME_Scale_Calculate(uint64_t elapsed_ms, uint32_t duration_ms, int32_t start_scale_fixed16, int32_t end_scale_fixed16, AME_EasingCurve curve) {
    if (duration_ms == 0 || elapsed_ms >= duration_ms) return end_scale_fixed16;
    int32_t progress = (int32_t)((elapsed_ms * 65536ULL) / duration_ms);
    int32_t eased = AME_EvaluateCurve(curve, progress);
    return AME_Interpolate(start_scale_fixed16, end_scale_fixed16, eased);
}

void AME_Position_Calculate(uint64_t elapsed_ms, uint32_t duration_ms, int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, AME_EasingCurve curve, int32_t* out_x, int32_t* out_y) {
    if (duration_ms == 0 || elapsed_ms >= duration_ms) {
        if (out_x) *out_x = end_x;
        if (out_y) *out_y = end_y;
        return;
    }
    int32_t progress = (int32_t)((elapsed_ms * 65536ULL) / duration_ms);
    int32_t eased = AME_EvaluateCurve(curve, progress);
    if (out_x) *out_x = AME_Interpolate(start_x, end_x, eased);
    if (out_y) *out_y = AME_Interpolate(start_y, end_y, eased);
}
