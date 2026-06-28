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
    return false; // Not handled by widgets
}
