#ifndef BOS_UI_CONTROLS_H
#define BOS_UI_CONTROLS_H

#include "bos_ui_core.h"
#include "platform/include/bos_window.h"

/* Button, ToggleButton, CheckBox, RadioButton */
typedef struct {
    BOS_UIElement base;
    char          text[128];
    void        (*on_click)(BOS_UIElement* sender);
} BOS_Button;

typedef struct {
    BOS_UIElement base;
    char          text[128];
    bool          is_checked;
    void        (*on_toggle)(BOS_UIElement* sender, bool checked);
} BOS_CheckBox, BOS_RadioButton, BOS_ToggleButton;

/* Label & Text Inputs */
typedef struct {
    BOS_UIElement base;
    char          text[256];
    uint32_t      text_role;
} BOS_Label;

typedef struct {
    BOS_UIElement base;
    char          text[256];
    char          placeholder[128];
    uint32_t      cursor_pos;
    bool          is_password;
    void        (*on_text_changed)(BOS_UIElement* sender, const char* new_text);
} BOS_TextBox;

/* Indicators & Controls */
typedef struct {
    BOS_UIElement base;
    int32_t       min_val;
    int32_t       max_val;
    int32_t       value;
    void        (*on_value_changed)(BOS_UIElement* sender, int32_t new_val);
} BOS_ProgressBar, BOS_Slider, BOS_ScrollBar;

/* Selectors & Containers */
typedef struct {
    BOS_UIElement base;
    char          items[32][64];
    uint32_t      item_count;
    int32_t       selected_index;
    void        (*on_select)(BOS_UIElement* sender, int32_t index);
} BOS_ComboBox, BOS_ListBox, BOS_ListView;

/* Window Control Wrapper */
typedef struct {
    BOS_UIElement    base;
    BOS_WindowHandle platform_handle;
    char             title[128];
    BOS_UIElement*   content;
} BOS_Window;

/* Panel Wrapper */
typedef struct {
    BOS_UIElement base;
} BOS_Panel;

/* Control Creator Functions */
BOS_Window*      BOS_Window_Create(int32_t x, int32_t y, uint32_t w, uint32_t h, const char* title);
void             BOS_Window_SetContent(BOS_Window* win, BOS_UIElement* content);
void             BOS_Window_Show(BOS_Window* win);

BOS_Panel*       BOS_Panel_Create(uint32_t bg_color);
BOS_Button*      BOS_Button_Create(const char* text, void (*on_click)(BOS_UIElement*));
BOS_Label*       BOS_Label_Create(const char* text);
BOS_TextBox*     BOS_TextBox_Create(const char* placeholder);
BOS_CheckBox*    BOS_CheckBox_Create(const char* text, void (*on_toggle)(BOS_UIElement*, bool));
BOS_ProgressBar* BOS_ProgressBar_Create(int32_t min_val, int32_t max_val);
BOS_ListView*    BOS_ListView_Create(void);
void             BOS_ListView_AddItem(BOS_ListView* list, const char* item);

#endif /* BOS_UI_CONTROLS_H */
