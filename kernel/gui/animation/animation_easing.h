#ifndef ANIMATION_EASING_H
#define ANIMATION_EASING_H

#include <stdint.h>

typedef enum {
    EASING_LINEAR,
    EASING_EASE_IN,
    EASING_EASE_OUT,
    EASING_EASE_IN_OUT,
    EASING_SMOOTHSTEP
} AnimEasing;

// t is fixed-point integer where 0 = 0.0 and 1000 = 1.0
// returns fixed-point integer where 0 = 0.0 and 1000 = 1.0
uint32_t easing_calculate(AnimEasing type, uint32_t t);

#endif
