#ifndef POINTER_BOUNDS_H
#define POINTER_BOUNDS_H

#include <stdint.h>

void pointer_bounds_init(uint32_t width, uint32_t height);
void pointer_bounds_update(uint32_t width, uint32_t height);
void pointer_bounds_clamp(int32_t* x, int32_t* y);

#endif
