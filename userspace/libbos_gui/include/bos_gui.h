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

void BOS_GUI_Init(void);
BOSWindow* BOS_CreateWindow(const char* title, int32_t x, int32_t y, int32_t width, int32_t height);
void BOS_ShowWindow(BOSWindow* window);
BOSPanel* BOS_CreatePanel(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t bg_color);
BOSButton* BOS_CreateButton(BOSWindow* window, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void));
BOSButton* BOS_CreateButtonInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void));
BOSLabel* BOS_CreateLabel(BOSWindow* window, const char* text, int32_t x, int32_t y, uint32_t color);
BOSProgressBar* BOS_CreateProgressBar(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height);
void BOS_ProgressBarSetValue(BOSProgressBar* pb, int val);

BOSCheckBox* BOS_CreateCheckBox(BOSWindow* window, const char* text, int32_t x, int32_t y, bool checked, void (*on_toggle)(bool));
bool BOS_CheckBoxIsChecked(BOSCheckBox* cb);

bool BOS_Internal_ProcessWidgetEvent(BOS_GUIEvent* event);

void BOS_Run(void);

#endif
