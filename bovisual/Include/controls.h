#ifndef BOVISUAL_CONTROLS_H
#define BOVISUAL_CONTROLS_H

#include "bovisual_types.h"

#include "geometry.h"
#include "layout.h"
#include "boscal_types.h"

typedef struct {
    BVDimension bounds_def_w;
    BVDimension bounds_def_h;
    BVAnchor anchor;
    BVRect bounds;
    BVPadding padding;
    BOVISUAL_Color bg_color;
    BOVISUAL_Color border_color;
    bool draw_border;
} BOVISUAL_Control_Panel;

typedef struct {
    BVDimension bounds_def_w;
    BVDimension bounds_def_h;
    BVAnchor anchor;
    BVRect bounds;
    BVPadding padding;
    const char* text;
    BOVISUAL_Color text_color;
    BOVISUAL_Color bg_color;
    bool transparent_bg;
    BVLayoutAlignment h_align;
    BVLayoutAlignment v_align;
} BOVISUAL_Control_Label;

typedef struct {
    BVDimension bounds_def_w;
    BVDimension bounds_def_h;
    BVAnchor anchor;
    BVRect bounds;
    BVPadding padding;
    const char* text;
    BOVISUAL_Color bg_color;
    BOVISUAL_Color hover_color;
    BOVISUAL_Color pressed_color;
    BOVISUAL_Color text_color;
    BOVISUAL_Color border_color;
    BVLayoutAlignment h_align;
    BVLayoutAlignment v_align;
    bool is_pressed;
    bool is_hovered;
    bool is_focused;
} BOVISUAL_Control_Button;

typedef struct {
    BVDimension bounds_def_w;
    BVDimension bounds_def_h;
    BVAnchor anchor;
    BVRect bounds;
    BVPadding padding;
    const char* text;
    BOVISUAL_Color bg_color;
    BOVISUAL_Color text_color;
    BOVISUAL_Color border_color;
    BVLayoutAlignment h_align;
    BVLayoutAlignment v_align;
    bool has_focus;
} BOVISUAL_Control_TextBox;

typedef struct {
    BVDimension bounds_def_w;
    BVDimension bounds_def_h;
    BVAnchor anchor;
    BVRect bounds;
    BVPadding padding;
    const uint8_t* bmp_data;
    uint32_t bmp_size;
} BOVISUAL_Control_Image;

typedef struct {
    BVDimension bounds_def_w;
    BVDimension bounds_def_h;
    BVAnchor anchor;
    BVRect bounds;
    BVPadding padding;
    int32_t min_value;
    int32_t max_value;
    int32_t current_value;
    BOVISUAL_Color bg_color;
    BOVISUAL_Color fill_color;
    BOVISUAL_Color border_color;
    bool draw_border;
} BOVISUAL_Control_ProgressBar;

// Note: Controls NEVER call Graphics or Drawing directly. They route through Renderer.

void BV_Panel_Render(const BOVISUAL_Control_Panel* panel);
void BV_Label_Render(const BOVISUAL_Control_Label* label);
void BV_Button_Render(const BOVISUAL_Control_Button* button);
void BV_TextBox_Render(const BOVISUAL_Control_TextBox* textbox);
void BV_Image_Render(const BOVISUAL_Control_Image* image);
void BV_ProgressBar_Render(const BOVISUAL_Control_ProgressBar* pbar);

#endif // BOVISUAL_CONTROLS_H
