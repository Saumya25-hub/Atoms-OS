#include "../Include/renderer.h"
#include "../Include/drawing.h"
#include "../Include/text.h"
#include "../Include/images.h"
#include "../Include/layout.h"
#include "../Include/geometry.h"
#include <stddef.h>

void BVRenderer_DrawPanel(const BOVISUAL_Control_Panel* panel) {
    if (!panel) return;

    BVLayoutResult layout = BV_CalculateLayout(panel->bounds, panel->padding, (BVTextMetrics){0,0,0,0,0}, BV_ALIGN_START, BV_ALIGN_START);

    // Draw background inside content bounds
    BOVISUAL_Draw_FilledRectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                  layout.content_bounds.width, layout.content_bounds.height, 
                                  panel->bg_color);

    if (panel->draw_border) {
        // Border gets drawn on the content bounds edge
        BOVISUAL_Draw_Rectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                layout.content_bounds.width, layout.content_bounds.height, 
                                panel->border_color);
    }
}

void BVRenderer_DrawLabel(const BOVISUAL_Control_Label* label) {
    if (!label) return;

    const BVFontMetrics* font = BV_GetDefaultFont();
    BVTextMetrics tMetrics = BV_TextMeasure(font, label->text);
    BVLayoutResult layout = BV_CalculateLayout(label->bounds, label->padding, tMetrics, label->h_align, label->v_align);

    if (!label->transparent_bg) {
        BOVISUAL_Draw_FilledRectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                      layout.content_bounds.width, layout.content_bounds.height, 
                                      label->bg_color);
    }

    if (label->text) {
        BOVISUAL_Draw_String(layout.text_bounds.x, layout.text_bounds.y, label->text, 
                             label->text_color, label->bg_color, label->transparent_bg, font);
    }
}

void BVRenderer_DrawButton(const BOVISUAL_Control_Button* button) {
    if (!button) return;

    const BVFontMetrics* font = BV_GetDefaultFont();
    BVTextMetrics tMetrics = BV_TextMeasure(font, button->text);
    BVLayoutResult layout = BV_CalculateLayout(button->bounds, button->padding, tMetrics, button->h_align, button->v_align);

    BOVISUAL_Color current_bg = button->bg_color;
    if (button->is_pressed) current_bg = button->pressed_color;
    else if (button->is_hovered) current_bg = button->hover_color;

    BOVISUAL_Draw_FilledRectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                  layout.content_bounds.width, layout.content_bounds.height, 
                                  current_bg);
                                  
    BOVISUAL_Draw_Rectangle(layout.content_bounds.x, layout.content_bounds.y, 
                            layout.content_bounds.width, layout.content_bounds.height, 
                            button->border_color);

    if (button->text) {
        BOVISUAL_Draw_String(layout.text_bounds.x, layout.text_bounds.y, button->text, 
                             button->text_color, current_bg, true, font);
    }
}

void BVRenderer_DrawTextBox(const BOVISUAL_Control_TextBox* textbox) {
    if (!textbox) return;

    const BVFontMetrics* font = BV_GetDefaultFont();
    BVTextMetrics tMetrics = BV_TextMeasure(font, textbox->text);
    BVLayoutResult layout = BV_CalculateLayout(textbox->bounds, textbox->padding, tMetrics, textbox->h_align, textbox->v_align);

    BOVISUAL_Draw_FilledRectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                  layout.content_bounds.width, layout.content_bounds.height, 
                                  textbox->bg_color);

    BOVISUAL_Color border = textbox->has_focus ? 0xFF0000FF : textbox->border_color;
    BOVISUAL_Draw_Rectangle(layout.content_bounds.x, layout.content_bounds.y, 
                            layout.content_bounds.width, layout.content_bounds.height, 
                            border);

    if (textbox->text) {
        BOVISUAL_Draw_String(layout.text_bounds.x, layout.text_bounds.y, textbox->text, 
                             textbox->text_color, textbox->bg_color, true, font);
    }
}

void BVRenderer_DrawImage(const BOVISUAL_Control_Image* image) {
    if (!image) return;
    
    BVLayoutResult layout = BV_CalculateLayout(image->bounds, image->padding, (BVTextMetrics){0,0,0,0,0}, BV_ALIGN_START, BV_ALIGN_START);
    
    if (image->bmp_data) {
        // Technically images should be aligned inside content bounds as well, 
        // but for now we draw at top left of content bounds
        BOVISUAL_Draw_BMP(layout.content_bounds.x, layout.content_bounds.y, image->bmp_data, image->bmp_size);
    }
}

void BVRenderer_DrawProgressBar(const BOVISUAL_Control_ProgressBar* pbar) {
    if (!pbar) return;

    BVLayoutResult layout = BV_CalculateLayout(pbar->bounds, pbar->padding, (BVTextMetrics){0,0,0,0,0}, BV_ALIGN_START, BV_ALIGN_START);

    // Draw background
    BOVISUAL_Draw_FilledRectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                  layout.content_bounds.width, layout.content_bounds.height, 
                                  pbar->bg_color);

    // Calculate fill width
    int32_t range = pbar->max_value - pbar->min_value;
    if (range > 0 && pbar->current_value > pbar->min_value) {
        int32_t val = pbar->current_value;
        if (val > pbar->max_value) val = pbar->max_value;
        
        int32_t fill_w = ((val - pbar->min_value) * layout.content_bounds.width) / range;
        if (fill_w > 0) {
            BOVISUAL_Draw_FilledRectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                          fill_w, layout.content_bounds.height, 
                                          pbar->fill_color);
        }
    }

    // Draw border
    if (pbar->draw_border) {
        BOVISUAL_Draw_Rectangle(layout.content_bounds.x, layout.content_bounds.y, 
                                layout.content_bounds.width, layout.content_bounds.height, 
                                pbar->border_color);
    }
}

