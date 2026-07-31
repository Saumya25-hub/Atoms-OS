#ifndef BOS_GUI_H
#define BOS_GUI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    BOS_GUI_EVENT_NONE = 0,
    BOS_GUI_EVENT_CLICK = 1,
    BOS_GUI_EVENT_CLOSE = 2,
    BOS_GUI_EVENT_KEY_DOWN = 3,
    BOS_GUI_EVENT_KEY_UP = 4,
    BOS_GUI_EVENT_MOUSE_MOVE = 5,
    BOS_GUI_EVENT_MOUSE_DOWN = 6,
    BOS_GUI_EVENT_MOUSE_UP = 7
} BOS_GUIEventType;

typedef struct {
    BOS_GUIEventType type;
    uint32_t control_id;
    uint32_t window_id;
    union {
        uint64_t user_callback;
        struct {
            uint32_t keycode;
            uint32_t modifiers;
            uint32_t ascii;
        } key;
        struct {
            int32_t x;
            int32_t y;
            uint32_t buttons;
        } mouse;
    };
} BOS_GUIEvent;

typedef struct {
    uint32_t id;
} BOSWindow;

typedef struct {
    uint32_t id;
} BOSPanel;

typedef struct {
    uint32_t id;
} BOSButton;

typedef struct {
    uint32_t id;
} BOSLabel;

typedef struct {
    uint32_t bg_panel_id;
    uint32_t fill_panel_id;
    int min, max, val;
    int w, h;
} BOSProgressBar;

typedef struct {
    uint32_t root_panel_id;
    uint32_t box_panel_id;
    uint32_t text_label_id;
    bool checked;
    void (*on_toggle)(bool);
} BOSCheckBox;

typedef struct {
    uint32_t bg_panel_id;
    uint32_t text_label_id;
    char text_buf[128];
} BOSTextBox;

typedef struct {
    uint32_t radio_button_id;
    uint32_t text_label_id;
    uint32_t group_id;
    bool selected;
    void (*on_select)(uint32_t group_id);
} BOSRadioButton;

typedef struct {
    uint32_t viewport_panel_id;
    uint32_t content_panel_id;
    uint32_t scrollbar_bg_id;
    uint32_t scrollbar_thumb_id;
    int32_t scroll_y;
    int32_t content_h;
} BOSScrollViewer;

void BOS_GUI_Init(void);
BOSWindow* BOS_CreateWindow(const char* title, int32_t x, int32_t y, int32_t width, int32_t height);
void BOS_ShowWindow(BOSWindow* window);

BOSPanel* BOS_CreatePanel(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t bg_color);
BOSPanel* BOS_CreatePanelInPanel(BOSPanel* parent, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t bg_color);

BOSButton* BOS_CreateButton(BOSWindow* window, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void));
BOSButton* BOS_CreateButtonInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void));

BOSLabel* BOS_CreateLabel(BOSWindow* window, const char* text, int32_t x, int32_t y, uint32_t color);
BOSLabel* BOS_CreateLabelInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, uint32_t color);

BOSProgressBar* BOS_CreateProgressBar(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height);
BOSProgressBar* BOS_CreateProgressBarInPanel(BOSPanel* panel, int32_t x, int32_t y, int32_t width, int32_t height);
void BOS_ProgressBarSetValue(BOSProgressBar* pb, int val);

BOSCheckBox* BOS_CreateCheckBox(BOSWindow* window, const char* text, int32_t x, int32_t y, bool checked, void (*on_toggle)(bool));
BOSCheckBox* BOS_CreateCheckBoxInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, bool checked, void (*on_toggle)(bool));
bool BOS_CheckBoxIsChecked(BOSCheckBox* cb);

BOSTextBox* BOS_CreateTextBoxInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, int32_t width, int32_t height);
BOSRadioButton* BOS_CreateRadioButtonInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, uint32_t group_id, bool selected, void (*on_select)(uint32_t group_id));
BOSScrollViewer* BOS_CreateScrollViewerInPanel(BOSPanel* panel, int32_t x, int32_t y, int32_t width, int32_t height, int32_t content_height);

void BOS_SetText(uint32_t control_id, const char* text);
void BOS_SetBounds(uint32_t control_id, int32_t x, int32_t y, int32_t width, int32_t height);

typedef enum {
    BOS_GRADIENT_NONE = 0,
    BOS_GRADIENT_VERTICAL = 1,
    BOS_GRADIENT_HORIZONTAL = 2
} BOSGradientMode;

// Generic Control Visual Styling APIs
void BOS_SetCornerRadius(uint32_t control_id, uint32_t radius);
void BOS_SetGradient(uint32_t control_id, uint32_t color_start, uint32_t color_end, BOSGradientMode mode);

// Strong-typed convenience wrappers for BOSWindow*, BOSPanel*, BOSButton*
void BOS_WindowSetCornerRadius(BOSWindow* win, uint32_t radius);
void BOS_PanelSetCornerRadius(BOSPanel* pnl, uint32_t radius);
void BOS_ButtonSetCornerRadius(BOSButton* btn, uint32_t radius);

void BOS_WindowSetGradient(BOSWindow* win, uint32_t color_start, uint32_t color_end, BOSGradientMode mode);
void BOS_PanelSetGradient(BOSPanel* pnl, uint32_t color_start, uint32_t color_end, BOSGradientMode mode);
void BOS_ButtonSetGradient(BOSButton* btn, uint32_t color_start, uint32_t color_end, BOSGradientMode mode);

bool BOS_Internal_ProcessWidgetEvent(BOS_GUIEvent* event);

void BOS_Run(void);

#endif
