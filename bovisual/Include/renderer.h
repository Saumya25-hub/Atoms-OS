#ifndef BOVISUAL_RENDERER_H
#define BOVISUAL_RENDERER_H

#include "controls.h"

// The Renderer sits between Controls and Drawing.
// Controls NEVER call Graphics directly. They call Renderer.
// Renderer calls Drawing. Drawing calls Graphics.

void BVRenderer_DrawPanel(const BOVISUAL_Control_Panel* panel);
void BVRenderer_DrawLabel(const BOVISUAL_Control_Label* label);
void BVRenderer_DrawButton(const BOVISUAL_Control_Button* button);
void BVRenderer_DrawTextBox(const BOVISUAL_Control_TextBox* textbox);
void BVRenderer_DrawImage(const BOVISUAL_Control_Image* image);
void BVRenderer_DrawProgressBar(const BOVISUAL_Control_ProgressBar* pbar);
void BVRenderer_DrawCursor(int32_t x, int32_t y);

#endif // BOVISUAL_RENDERER_H
