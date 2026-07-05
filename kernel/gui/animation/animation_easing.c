#include "animation_easing.h"

// Math in fixed point. 1.0 = 1000.

uint32_t easing_calculate(AnimEasing type, uint32_t t) {
    if (t > 1000) t = 1000;

    switch (type) {
        case EASING_LINEAR:
            return t;
            
        case EASING_EASE_IN:
            return (t * t) / 1000;
            
        case EASING_EASE_OUT:
            return (t * (2000 - t)) / 1000;
            
        case EASING_EASE_IN_OUT:
            if (t < 500) {
                return (2 * t * t) / 1000;
            } else {
                return (1000000 - (2000 - 2 * t) * (1000 - t)) / 1000;
            }
            
        case EASING_SMOOTHSTEP:
            // t * t * (3 - 2 * t)
            // (t * t / 1000) * (3000 - 2 * t) / 1000
            return ((t * t / 1000) * (3000 - 2 * t)) / 1000;
            
        default:
            return t;
    }
}
