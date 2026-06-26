#include "../Include/controls.h"
#include "../Include/renderer.h"

// Controls NEVER call Drawing or Graphics directly.
// They delegate the task to the Renderer which orchestrates the visual effects.

void BV_Panel_Render(const BOVISUAL_Control_Panel* panel) {
    BVRenderer_DrawPanel(panel);
}

void BV_Label_Render(const BOVISUAL_Control_Label* label) {
    BVRenderer_DrawLabel(label);
}

void BV_Button_Render(const BOVISUAL_Control_Button* button) {
    BVRenderer_DrawButton(button);
}

void BV_TextBox_Render(const BOVISUAL_Control_TextBox* textbox) {
    BVRenderer_DrawTextBox(textbox);
}

void BV_Image_Render(const BOVISUAL_Control_Image* image) {
    if (!image) return;
    BVRenderer_DrawImage(image);
}

void BV_ProgressBar_Render(const BOVISUAL_Control_ProgressBar* pbar) {
    if (!pbar) return;
    BVRenderer_DrawProgressBar(pbar);
}
