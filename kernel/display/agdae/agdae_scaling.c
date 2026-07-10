/**
 * @file agdae_scaling.c
 * @brief ATOMS OS Display Adaptation Engine - Integer Scaling Pipeline
 */

#include "agdae.h"

extern AGDAE_Metrics g_agdae_metrics;

int32_t AGDAE_Scale(int32_t value) {
    if (!g_agdae_metrics.is_scaled) {
        return value;
    }
    
    /* Integer-first scaling: value * percent / 100 */
    /* Example: 40px at 125% -> 40 * 125 / 100 = 50px */
    return (value * (int32_t)g_agdae_metrics.scale_factor_pct) / 100;
}

int32_t AGDAE_Unscale(int32_t scaled_value) {
    if (!g_agdae_metrics.is_scaled) {
        return scaled_value;
    }
    
    /* Inverse scaling: scaled_value * 100 / percent */
    return (scaled_value * 100) / (int32_t)g_agdae_metrics.scale_factor_pct;
}
