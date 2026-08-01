#include "../include/bwe.h"

// Stubs/Helpers
extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern bwe_error_t BOS_ListView_AddItem(uint32_t list_id, const char* item);
extern bwe_error_t BOS_TreeView_AddNode(uint32_t tree_id, const char* name, int32_t parent_node_idx, int32_t* out_node_idx);

uint32_t s_demo_win_id = 0;
static uint32_t s_btn_id = 0;
static uint32_t s_chk_id = 0;
static uint32_t s_progress_id = 0;

static void on_demo_button_click(uint32_t btn_id) {
    display_print("[DEMO_APP] Button Clicked! ID #");
    display_print_dec(btn_id);
    display_print("\n");

    BWE_Window* win = BWE_GetWindow(s_progress_id);
    if (win) {
        win->control_data.progressbar.value += 10;
        if (win->control_data.progressbar.value > win->control_data.progressbar.max) {
            win->control_data.progressbar.value = win->control_data.progressbar.min;
        }
        BWE_InvalidateWindow(s_progress_id);
    }
}

static void on_demo_checkbox_toggle(uint32_t chk_id, bool checked) {
    (void)chk_id;
    display_print("[DEMO_APP] Checkbox Toggled to ");
    display_print(checked ? "CHECKED" : "UNCHECKED");
    display_print("\n");
}

static void on_canvas_custom_paint(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip) {
    (void)canvas_id;
    (void)clip;
    
    BWE_Window* self = BWE_GetWindow(canvas_id);
    if (!self) return;

    BWE_Rect b = self->screen_bounds;

    BWE_FillRect(fb, b.x + 2, b.y + 2, b.width - 4, b.height - 4, 0xFFFFFFFF);
    BWE_DrawRect(fb, b.x, b.y, b.width, b.height, 0xFFEF4444, 2);
    BWE_DrawLine(fb, b.x + 4, b.y + 4, b.x + b.width - 4, b.y + b.height - 4, 0xFF3B82F6);
    BWE_DrawLine(fb, b.x + b.width - 4, b.y + 4, b.x + 4, b.y + b.height - 4, 0xFF10B981);
}

void BWE_DemoApp_Initialize(void) {
    bwe_error_t err = BOS_CreateWindow(150, 100, 600, 480, "ATOMS OS BWE V2.0 GUI Demo", &s_demo_win_id);
    if (err != BWE_SUCCESS) return;

    uint32_t panel_id;
    BOS_CreatePanel(s_demo_win_id, 10, 10, 580, 50, 0xFFE2E8F0, &panel_id);
    uint32_t label_id;
    BOS_CreateLabel(panel_id, 15, 17, "Controls Sandbox:", 0xFF0F172A, &label_id);
    BOS_CreateButton(panel_id, 180, 10, 120, 30, "Click Me!", on_demo_button_click, &s_btn_id);
    BOS_CreateCheckbox(panel_id, 320, 10, 150, 30, "Show Grid", on_demo_checkbox_toggle, &s_chk_id);

    uint32_t cv_id;
    BOS_CreateCanvas(s_demo_win_id, 420, 140, 150, 120, on_canvas_custom_paint, &cv_id);

    BOS_Show(s_demo_win_id);
    BOS_SetFocus(s_demo_win_id);
}
