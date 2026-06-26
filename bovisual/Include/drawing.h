#ifndef BOVISUAL_DRAWING_H
#define BOVISUAL_DRAWING_H

#include "bovisual_types.h"

// Note: These functions are internal to the BOVISUAL engine and 
// will be used by the Controls module.

void BOVISUAL_Draw_Line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, BOVISUAL_Color color);
void BOVISUAL_Draw_Rectangle(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color);
void BOVISUAL_Draw_FilledRectangle(int32_t x, int32_t y, int32_t width, int32_t height, BOVISUAL_Color color);
void BOVISUAL_Draw_Circle(int32_t x0, int32_t y0, int32_t radius, BOVISUAL_Color color);
void BOVISUAL_Draw_Ellipse(int32_t x0, int32_t y0, int32_t rx, int32_t ry, BOVISUAL_Color color);

#endif // BOVISUAL_DRAWING_H
