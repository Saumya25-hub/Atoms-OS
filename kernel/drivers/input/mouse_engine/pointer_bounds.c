#include "pointer_bounds.h"
#include "pointer_diag.h"

static int32_t max_x = 0;
static int32_t max_y = 0;

void pointer_bounds_init(uint32_t width, uint32_t height) {
    pointer_bounds_update(width, height);
}

void pointer_bounds_update(uint32_t width, uint32_t height) {
    if (width > 0) max_x = (int32_t)width - 1;
    else max_x = 0;
    
    if (height > 0) max_y = (int32_t)height - 1;
    else max_y = 0;
}

void pointer_bounds_clamp(int32_t* x, int32_t* y) {
    bool clamped = false;
    
    if (*x < 0) {
        *x = 0;
        clamped = true;
    }
    if (*y < 0) {
        *y = 0;
        clamped = true;
    }
    if (*x > max_x) {
        *x = max_x;
        clamped = true;
    }
    if (*y > max_y) {
        *y = max_y;
        clamped = true;
    }
    
    if (clamped) {
        pointer_diag_inc_clamp();
    }
}
