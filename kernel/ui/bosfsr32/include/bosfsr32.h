#ifndef BOSFSR32_H
#define BOSFSR32_H

#include "bosfsr_core.h"
#include "bosfsr_graphics.h"
#include "bosfsr_containers.h"
#include "bosfsr_controls.h"

void bosfsr32_init(void);
void bosfsr32_render_window(BWE_Window* win, BOSFSR_Control* root);

#endif /* BOSFSR32_H */
