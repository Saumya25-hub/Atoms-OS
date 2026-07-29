#ifndef RENDER_AUDIT_H
#define RENDER_AUDIT_H

#include "kernel/core/lib/include/stdint.h"

void audit_log_draw(const char* func, int x, int y, int w, int h, int r, uint32_t color);

#endif


