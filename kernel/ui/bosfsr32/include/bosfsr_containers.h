#ifndef BOSFSR_CONTAINERS_H
#define BOSFSR_CONTAINERS_H

#include "bosfsr_core.h"

BOSFSR_Control* bosfsr_create_panel(const char* name, int x, int y, int w, int h, uint32_t bg_color);
BOSFSR_Control* bosfsr_create_rounded_panel(const char* name, int x, int y, int w, int h, int radius, uint32_t bg_color, uint32_t border_color, int border_w);
BOSFSR_Control* bosfsr_create_gradient_panel(const char* name, int x, int y, int w, int h, uint32_t start_col, uint32_t end_col, bool vertical);

#endif /* BOSFSR_CONTAINERS_H */
