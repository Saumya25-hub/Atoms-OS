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

void BOS_GUI_Init(void) {
    win_count = 0;
    panel_count = 0;
    button_count = 0;
    label_count = 0;
    pb_count = 0;
    cb_count = 0;
}

BOSWindow* BOS_CreateWindow(const char* title, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (win_count >= 16) return 0;
    uint32_t id = sys_gui_create_window(x, y, width, height, title);
    if (!id) return 0;
    BOSWindow* win = &window_pool[win_count++];
    win->id = id;
    return win;
}

BOSButton* BOS_CreateButton(BOSWindow* window, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void)) {
    if (!window || button_count >= 128) return 0;
    uint32_t id = sys_gui_create_button(window->id, x, y, width, height, text, on_click);
    if (!id) return 0;
    BOSButton* btn = &button_pool[button_count++];
    btn->id = id;
    return btn;
}

BOSButton* BOS_CreateButtonInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, int32_t width, int32_t height, void (*on_click)(void)) {
    if (!panel || button_count >= 128) return 0;
    uint32_t id = sys_gui_create_button(panel->id, x, y, width, height, text, on_click);
    if (!id) return 0;
    BOSButton* btn = &button_pool[button_count++];
    btn->id = id;
    return btn;
}

BOSPanel* BOS_CreatePanel(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t bg_color) {
    if (!window || panel_count >= 64) return 0;
    uint32_t id = sys_gui_create_panel(window->id, x, y, width, height, bg_color);
    if (!id) return 0;
    BOSPanel* pnl = &panel_pool[panel_count++];
    pnl->id = id;
    return pnl;
}

BOSLabel* BOS_CreateLabel(BOSWindow* window, const char* text, int32_t x, int32_t y, uint32_t color) {
    if (!window || label_count >= 128) return 0;
    uint32_t id = sys_gui_create_label(window->id, x, y, text, color);
    if (!id) return 0;
    BOSLabel* lbl = &label_pool[label_count++];
    lbl->id = id;
    return lbl;
}

void BOS_ShowWindow(BOSWindow* window) {
    if (window) sys_gui_show_window(window->id);
}

BOSProgressBar* BOS_CreateProgressBar(BOSWindow* window, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!window || pb_count >= 32) return 0;
    
    // Background panel (dark gray)
    uint32_t bg_id = sys_gui_create_panel(window->id, x, y, width, height, 0xFF333333);
    if (!bg_id) return 0;
    
    // Fill panel (green)
    uint32_t fill_id = sys_gui_create_panel(bg_id, 0, 0, 0, height, 0xFF10B981);
    
    BOSProgressBar* pb = &pb_pool[pb_count++];
    pb->bg_panel_id = bg_id;
    pb->fill_panel_id = fill_id;
    pb->min = 0;
    pb->max = 100;
    pb->val = 0;
    pb->w = width;
    pb->h = height;
    
    return pb;
}

void BOS_ProgressBarSetValue(BOSProgressBar* pb, int val) {
    if (!pb) return;
    if (val < pb->min) val = pb->min;
    if (val > pb->max) val = pb->max;
    pb->val = val;
    
    int fill_w = (pb->w * (val - pb->min)) / (pb->max - pb->min);
    sys_gui_set_bounds(pb->fill_panel_id, 0, 0, fill_w, pb->h);
}

BOSCheckBox* BOS_CreateCheckBox(BOSWindow* window, const char* text, int32_t x, int32_t y, bool checked, void (*on_toggle)(bool)) {
    if (!window || cb_count >= 32) return 0;
    
    // Root panel (transparent ideally, but we use a matching bg color or just use it as container)
    // Actually we can create the box and label directly on the window!
    // Box: 20x20
    uint32_t box_color = checked ? 0xFF3B82F6 : 0xFFFFFFFF; // Blue if checked, white if not
    uint32_t box_id = sys_gui_create_button(window->id, x, y, 20, 20, checked ? "X" : "", 0); // Using button to capture click easily!
    
    // Label next to it
    uint32_t lbl_id = sys_gui_create_label(window->id, x + 30, y + 2, text, 0xFFFFFFFF);
    
    BOSCheckBox* cb = &cb_pool[cb_count++];
    cb->root_panel_id = 0; 
    cb->box_panel_id = box_id;
    cb->text_label_id = lbl_id;
    cb->checked = checked;
    cb->on_toggle = on_toggle;
    
    return cb;
}

bool BOS_CheckBoxIsChecked(BOSCheckBox* cb) {
    return cb ? cb->checked : false;
}

BOSPanel* BOS_CreatePanelInPanel(BOSPanel* parent, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t bg_color) {
    if (!parent || panel_count >= 64) return 0;
    uint32_t id = sys_gui_create_panel(parent->id, x, y, width, height, bg_color);
    if (!id) return 0;
    BOSPanel* pnl = &panel_pool[panel_count++];
    pnl->id = id;
    return pnl;
}

BOSLabel* BOS_CreateLabelInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, uint32_t color) {
    if (!panel || label_count >= 128) return 0;
    uint32_t id = sys_gui_create_label(panel->id, x, y, text, color);
    if (!id) return 0;
    BOSLabel* lbl = &label_pool[label_count++];
    lbl->id = id;
    return lbl;
}

BOSProgressBar* BOS_CreateProgressBarInPanel(BOSPanel* panel, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!panel || pb_count >= 32) return 0;
    uint32_t bg_id = sys_gui_create_panel(panel->id, x, y, width, height, 0xFF333333);
    if (!bg_id) return 0;
    uint32_t fill_id = sys_gui_create_panel(bg_id, 0, 0, 0, height, 0xFF10B981);
    
    BOSProgressBar* pb = &pb_pool[pb_count++];
    pb->bg_panel_id = bg_id;
    pb->fill_panel_id = fill_id;
    pb->min = 0;
    pb->max = 100;
    pb->val = 0;
    pb->w = width;
    pb->h = height;
    return pb;
}

BOSCheckBox* BOS_CreateCheckBoxInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, bool checked, void (*on_toggle)(bool)) {
    if (!panel || cb_count >= 32) return 0;
    uint32_t box_id = sys_gui_create_button(panel->id, x, y, 20, 20, checked ? "X" : "", 0);
    uint32_t lbl_id = sys_gui_create_label(panel->id, x + 30, y + 2, text, 0xFFFFFFFF);
    
    BOSCheckBox* cb = &cb_pool[cb_count++];
    cb->root_panel_id = 0; 
    cb->box_panel_id = box_id;
    cb->text_label_id = lbl_id;
    cb->checked = checked;
    cb->on_toggle = on_toggle;
    return cb;
}

static BOSTextBox tb_pool[32];
static int tb_count = 0;

BOSTextBox* BOS_CreateTextBoxInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (!panel || tb_count >= 32) return 0;
    uint32_t bg_id = sys_gui_create_panel(panel->id, x, y, width, height, 0xFF0F172A);
    sys_gui_set_corner_radius(bg_id, 4);
    uint32_t lbl_id = sys_gui_create_label(bg_id, 8, (height - 14) / 2, text ? text : "", 0xFFE2E8F0);
    
    BOSTextBox* tb = &tb_pool[tb_count++];
    tb->bg_panel_id = bg_id;
    tb->text_label_id = lbl_id;
    int i = 0;
    if (text) {
        for (; text[i] && i < 127; i++) tb->text_buf[i] = text[i];
    }
    tb->text_buf[i] = '\0';
    return tb;
}

static BOSRadioButton rb_pool[32];
static int rb_count = 0;

BOSRadioButton* BOS_CreateRadioButtonInPanel(BOSPanel* panel, const char* text, int32_t x, int32_t y, uint32_t group_id, bool selected, void (*on_select)(uint32_t group_id)) {
    if (!panel || rb_count >= 32) return 0;
    uint32_t btn_id = sys_gui_create_button(panel->id, x, y, 18, 18, selected ? "O" : "", 0);
    sys_gui_set_corner_radius(btn_id, 9); // Circular radio button
    uint32_t lbl_id = sys_gui_create_label(panel->id, x + 28, y + 1, text, 0xFFFFFFFF);
    
    BOSRadioButton* rb = &rb_pool[rb_count++];
    rb->radio_button_id = btn_id;
    rb->text_label_id = lbl_id;
    rb->group_id = group_id;
    rb->selected = selected;
    rb->on_select = on_select;
    return rb;
}

static BOSScrollViewer sv_pool[16];
static int sv_count = 0;

BOSScrollViewer* BOS_CreateScrollViewerInPanel(BOSPanel* panel, int32_t x, int32_t y, int32_t width, int32_t height, int32_t content_height) {
    if (!panel || sv_count >= 16) return 0;
    uint32_t vp_id = sys_gui_create_panel(panel->id, x, y, width - 15, height, 0xFF1E293B);
    uint32_t content_id = sys_gui_create_panel(vp_id, 0, 0, width - 15, content_height, 0xFF1E293B);
    
    uint32_t sb_bg = sys_gui_create_panel(panel->id, x + width - 14, y, 14, height, 0xFF0F172A);
    uint32_t sb_thumb = sys_gui_create_button(sb_bg, 0, 0, 14, 40, "", 0);
    sys_gui_set_corner_radius(sb_thumb, 4);
    
    BOSScrollViewer* sv = &sv_pool[sv_count++];
    sv->viewport_panel_id = vp_id;
    sv->content_panel_id = content_id;
    sv->scrollbar_bg_id = sb_bg;
    sv->scrollbar_thumb_id = sb_thumb;
    sv->scroll_y = 0;
    sv->content_h = content_height;
    return sv;
}

void BOS_SetText(uint32_t control_id, const char* text) {
    if (control_id && text) {
        sys_gui_set_text(control_id, text);
    }
}

void BOS_SetBounds(uint32_t control_id, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (control_id) {
        sys_gui_set_bounds(control_id, x, y, width, height);
    }
}

bool BOS_Internal_ProcessWidgetEvent(BOS_GUIEvent* event) {
    if (event->type != BOS_GUI_EVENT_CLICK) return false;
    
    for (int i = 0; i < cb_count; i++) {
        if (cb_pool[i].box_panel_id == event->control_id) {
            cb_pool[i].checked = !cb_pool[i].checked;
            sys_gui_set_text(cb_pool[i].box_panel_id, cb_pool[i].checked ? "X" : "");
            if (cb_pool[i].on_toggle) {
                cb_pool[i].on_toggle(cb_pool[i].checked);
            }
            return true; // Handled
        }
    }
    
    for (int i = 0; i < rb_count; i++) {
        if (rb_pool[i].radio_button_id == event->control_id) {
            uint32_t grp = rb_pool[i].group_id;
            for (int j = 0; j < rb_count; j++) {
                if (rb_pool[j].group_id == grp) {
                    rb_pool[j].selected = (j == i);
                    sys_gui_set_text(rb_pool[j].radio_button_id, rb_pool[j].selected ? "O" : "");
                }
            }
            if (rb_pool[i].on_select) {
                rb_pool[i].on_select(grp);
            }
            return true;
        }
    }
    
    return false; // Not handled by widgets
}

void BOS_SetCornerRadius(uint32_t control_id, uint32_t radius) {
    if (control_id) {
        sys_gui_set_corner_radius(control_id, radius);
    }
}

void BOS_SetGradient(uint32_t control_id, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) {
    if (control_id) {
        sys_gui_set_gradient(control_id, color_start, color_end, (uint8_t)mode);
    }
}

void BOS_WindowSetCornerRadius(BOSWindow* win, uint32_t radius) {
    if (win) BOS_SetCornerRadius(win->id, radius);
}

void BOS_PanelSetCornerRadius(BOSPanel* pnl, uint32_t radius) {
    if (pnl) BOS_SetCornerRadius(pnl->id, radius);
}

void BOS_ButtonSetCornerRadius(BOSButton* btn, uint32_t radius) {
    if (btn) BOS_SetCornerRadius(btn->id, radius);
}

void BOS_WindowSetGradient(BOSWindow* win, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) {
    if (win) BOS_SetGradient(win->id, color_start, color_end, mode);
}

void BOS_PanelSetGradient(BOSPanel* pnl, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) {
    if (pnl) BOS_SetGradient(pnl->id, color_start, color_end, mode);
}

void BOS_ButtonSetGradient(BOSButton* btn, uint32_t color_start, uint32_t color_end, BOSGradientMode mode) {
    if (btn) BOS_SetGradient(btn->id, color_start, color_end, mode);
}
