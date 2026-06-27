#ifndef BOS_EXPLORER_H
#define BOS_EXPLORER_H

#include <stdint.h>
#include "kernel/BOSurface/Core/surface.h"

// Lifecycle methods (for App Manager)
bwe_error_t explorer_init(uint32_t* out_win);
void explorer_exit(void);

#endif // BOS_EXPLORER_H
