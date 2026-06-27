#include "pointer_filter.h"
#include "pointer_diag.h"

void pointer_filter_init(void) {
    // No-op for now
}

void pointer_filter_apply(int32_t raw_dx, int32_t raw_dy, int32_t* out_dx, int32_t* out_dy) {
    // Basic passthrough as per directive (do not scale to prevent VBox drift)
    *out_dx = raw_dx;
    *out_dy = raw_dy;
    
    pointer_diag_update_filtered(*out_dx, *out_dy);
}
