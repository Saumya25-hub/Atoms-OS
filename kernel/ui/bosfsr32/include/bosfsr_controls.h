#ifndef BOSFSR_CONTROLS_H
#define BOSFSR_CONTROLS_H

#include "bosfsr_core.h"

BOSFSR_Control* bosfsr_create_button(const char* name, int x, int y, int w, int h, const char* text, uint32_t bg_color);
BOSFSR_Control* bosfsr_create_label(const char* name, int x, int y, int w, int h, const char* text, uint32_t fg_color);
BOSFSR_Control* bosfsr_create_checkbox(const char* name, int x, int y, int w, int h, const char* text, bool is_checked);
BOSFSR_Control* bosfsr_create_radiobutton(const char* name, int x, int y, int w, int h, const char* text, bool is_checked);
BOSFSR_Control* bosfsr_create_progressbar(const char* name, int x, int y, int w, int h, int value);
BOSFSR_Control* bosfsr_create_gridview(const char* name, int x, int y, int w, int h);
BOSFSR_Control* bosfsr_create_image(const char* name, int x, int y, int w, int h);

#endif /* BOSFSR_CONTROLS_H */
