#include "../include/bos_gui.h"
#include "../include/syscalls_gui.h"

static BOSWindow window_pool[16];
static int win_count = 0;

static BOSPanel panel_pool[64];
static int panel_count = 0;

static BOSButton button_pool[128];
static int button_count = 0;

static BOSLabel label_pool[128];
static int label_count = 0;

static BOSProgressBar pb_pool[32];
static int pb_count = 0;

static BOSCheckBox cb_pool[32];
static int cb_count = 0;

static BOSTextBox tb_pool[32];
static int tb_count = 0;

static BOSRadioButton rb_pool[32];
static int rb_count = 0;

static BOSScrollViewer sv_pool[16];
static int sv_count = 0;

void BOS_GUI_Init(void) {
    win_count = 0;
    panel_count = 0;
    button_count = 0;
    label_count = 0;
    pb_count = 0;
    cb_count = 0;
    tb_count = 0;
    rb_count = 0;
    sv_count = 0;
}

BOSWindow* BOS_CreateWindow(const char* title, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (win_count >= 16) return 0;
    uint32_t id = sys_gui_create_window(x, y, width, height, 0, title);
    if (!id) return 0;
    BOSWindow* win = &window_pool[win_count++];
    win->id = id;
    return win;
}

void BOS_ShowWindow(BOSWindow* window) {
    if (window) sys_gui_show_window(window->id, true);
}

BOSPanel* BOS_CreatePanel(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t bg_color) {
    (void)window; (void)x; (void)y; (void)width; (void)height; (void)bg_color;
    if (panel_count >= 64) return 0;
    BOSPanel* pnl = &panel_pool[panel_count++];
    pnl->id = 100 + panel_count;
    return pnl;
}

BOSPanel* BOS_CreatePanelInPanel(BOSPanel* parent, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t bg_color) {
    (void)parent; (void)x; (void)y; (void)width; (void)height; (void)bg_color;
    if (panel_count >= 64) return 0;
    BOSPanel* pnl = &panel_pool[panel_count++];
    pnl->id = 100 + panel_count;
    return pnl;
}

BOSButton* BOS_CreateButton(BOSWindow* window, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void)) {
    (void)window; (void)text; (void)x; (void)y; (void)width; (void)height; (void)on_click;
    if (button_count >= 128) return 0;
    BOSButton* btn = &button_pool[button_count++];
    btn->id = 200 + button_count;
    return btn;
}

BOSButton* BOS_CreateButtonInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void)) {
    (void)panel; (void)text; (void)x; (void)y; (void)width; (void)height; (void)on_click;
    if (button_count >= 128) return 0;
    BOSButton* btn = &button_pool[button_count++];
    btn->id = 200 + button_count;
    return btn;
}

BOSLabel* BOS_CreateLabel(BOSWindow* window, const char* text, int32_t x, int32_t y, uint32_t color) {
    (void)window; (void)text; (void)x; (void)y; (void)color;
    if (label_count >= 128) return 0;
    BOSLabel* lbl = &label_pool[label_count++];
    lbl->id = 300 + label_count;
    return lbl;
}

BOSLabel* BOS_CreateLabelInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, uint32_t color) {
    (void)panel; (void)text; (void)x; (void)y; (void)color;
    if (label_count >= 128) return 0;
    BOSLabel* lbl = &label_pool[label_count++];
    lbl->id = 300 + label_count;
    return lbl;
}

BOSProgressBar* BOS_CreateProgressBar(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)window; (void)x; (void)y; (void)width; (void)height;
    if (pb_count >= 32) return 0;
    BOSProgressBar* pb = &pb_pool[pb_count++];
    pb->bg_panel_id = 400 + pb_count;
    pb->fill_panel_id = 450 + pb_count;
    pb->min = 0; pb->max = 100; pb->val = 0; pb->w = width; pb->h = height;
    return pb;
}

BOSProgressBar* BOS_CreateProgressBarInPanel(BOSPanel* panel, int32_t x, int32_t y, int32_t width, int32_t height) {
    return BOS_CreateProgressBar(0, x, y, width, height);
}

void BOS_ProgressBarSetValue(BOSProgressBar* pb, int val) {
    if (!pb) return;
    pb->val = val;
}

BOSCheckBox* BOS_CreateCheckBox(BOSWindow* window, const char* text, int32_t x, int32_t y, bool checked, void (*on_toggle)(bool)) {
    (void)window; (void)text; (void)x; (void)y;
    if (cb_count >= 32) return 0;
    BOSCheckBox* cb = &cb_pool[cb_count++];
    cb->box_panel_id = 500 + cb_count;
    cb->text_label_id = 550 + cb_count;
    cb->checked = checked;
    cb->on_toggle = on_toggle;
    return cb;
}

BOSCheckBox* BOS_CreateCheckBoxInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, bool checked, void (*on_toggle)(bool)) {
    return BOS_CreateCheckBox(0, text, x, y, checked, on_toggle);
}

bool BOS_CheckBoxIsChecked(BOSCheckBox* cb) {
    return cb ? cb->checked : false;
}

BOSTextBox* BOS_CreateTextBoxInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)panel; (void)x; (void)y; (void)width; (void)height;
    if (tb_count >= 32) return 0;
    BOSTextBox* tb = &tb_pool[tb_count++];
    tb->bg_panel_id = 600 + tb_count;
    tb->text_label_id = 650 + tb_count;
    int i = 0;
    if (text) {
        for (; text[i] && i < 127; i++) tb->text_buf[i] = text[i];
    }
    tb->text_buf[i] = '\0';
    return tb;
}

BOSRadioButton* BOS_CreateRadioButtonInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, uint32_t group_id, bool selected, void (*on_select)(uint32_t group_id)) {
    (void)panel; (void)text; (void)x; (void)y;
    if (rb_count >= 32) return 0;
    BOSRadioButton* rb = &rb_pool[rb_count++];
    rb->radio_button_id = 700 + rb_count;
    rb->text_label_id = 750 + rb_count;
    rb->group_id = group_id;
    rb->selected = selected;
    rb->on_select = on_select;
    return rb;
}

BOSScrollViewer* BOS_CreateScrollViewerInPanel(BOSPanel* panel, int32_t x, int32_t y, int32_t width, int32_t height, int32_t content_height) {
    (void)panel; (void)x; (void)y; (void)width; (void)height; (void)content_height;
    if (sv_count >= 16) return 0;
    BOSScrollViewer* sv = &sv_pool[sv_count++];
    sv->viewport_panel_id = 800 + sv_count;
    sv->content_panel_id = 850 + sv_count;
    sv->scrollbar_bg_id = 900 + sv_count;
    sv->scrollbar_thumb_id = 950 + sv_count;
    sv->scroll_y = 0;
    sv->content_h = content_height;
    return sv;
}

void BOS_SetText(uint32_t control_id, const char* text) {
    (void)control_id; (void)text;
}

void BOS_SetBounds(uint32_t control_id, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (control_id < 100) {
        sys_gui_set_bounds(control_id, x, y, width, height);
    }
}

void BOS_SetCornerRadius(uint32_t control_id, uint32_t radius) {
    (void)control_id; (void)radius;
}

void BOS_SetGradient(uint32_t control_id, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) {
    (void)control_id; (void)color_start; (void)color_end; (void)mode;
}

void BOS_WindowSetCornerRadius(BOSWindow* win, uint32_t radius) { (void)win; (void)radius; }
void BOS_PanelSetCornerRadius(BOSPanel* pnl, uint32_t radius) { (void)pnl; (void)radius; }
void BOS_ButtonSetCornerRadius(BOSButton* btn, uint32_t radius) { (void)btn; (void)radius; }

void BOS_WindowSetGradient(BOSWindow* win, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) { (void)win; (void)color_start; (void)color_end; (void)mode; }
void BOS_PanelSetGradient(BOSPanel* pnl, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) { (void)pnl; (void)color_start; (void)color_end; (void)mode; }
void BOS_ButtonSetGradient(BOSButton* btn, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) { (void)btn; (void)color_start; (void)color_end; (void)mode; }

bool BOS_Internal_ProcessWidgetEvent(BOS_GUIEvent* event) { (void)event; return false; }
