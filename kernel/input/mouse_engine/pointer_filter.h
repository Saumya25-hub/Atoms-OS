#ifndef POINTER_FILTER_H
#define POINTER_FILTER_H

#include <stdint.h>

void pointer_filter_init(void);
void pointer_filter_apply(int32_t raw_dx, int32_t raw_dy, int32_t* out_dx, int32_t* out_dy);

#endif
