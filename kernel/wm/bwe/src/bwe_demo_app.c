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

    // Increment progress bar value dynamically on button click
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

    // Fill white canvas interior
    BWE_FillRect(fb, b.x + 2, b.y + 2, b.width - 4, b.height - 4, 0xFFFFFFFF);

    // Draw canvas border
    BWE_DrawRect(fb, b.x, b.y, b.width, b.height, 0xFFEF4444, 2); // Red boundary border

    // Draw crossed diagonals
    BWE_DrawLine(fb, b.x + 4, b.y + 4, b.x + b.width - 4, b.y + b.height - 4, 0xFF3B82F6); // Blue line
    BWE_DrawLine(fb, b.x + b.width - 4, b.y + 4, b.x + 4, b.y + b.height - 4, 0xFF10B981); // Green line
}

void BWE_DemoApp_Initialize(void) {
    bwe_error_t err;

    // Create Main Demo Window
    err = BOS_CreateWindow(150, 100, 600, 480, "ATOMS OS BWE V2.0 GUI Demo", &s_demo_win_id);
    if (err != BWE_SUCCESS) {
        display_print("[DEMO_APP] Failed to create main demo window\n");
        return;
    }

    BWE_Window* win = BWE_GetWindow(s_demo_win_id);
    if (win) {
        win->padding.left = 10;
        win->padding.top = 10;
        win->padding.right = 10;
        win->padding.bottom = 10;
    }

    // 1. Create a Top Panel
    uint32_t panel_id;
    BOS_CreatePanel(s_demo_win_id, 10, 10, 580, 50, 0xFFE2E8F0, &panel_id);
    BWE_Window* p_win = BWE_GetWindow(panel_id);
    if (p_win) {
        p_win->anchor_flags = BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_RIGHT;
    }

    // 2. Create Label inside Panel
    uint32_t label_id;
    BOS_CreateLabel(panel_id, 15, 17, "Controls Sandbox:", 0xFF0F172A, &label_id);

    // 3. Create Button inside Panel
    BOS_CreateButton(panel_id, 180, 10, 120, 30, "Click Me!", on_demo_button_click, &s_btn_id);

    // 4. Create Checkbox inside Panel
    BOS_CreateCheckbox(panel_id, 320, 10, 150, 30, "Show Grid", on_demo_checkbox_toggle, &s_chk_id);

    // 5. Create Textbox below Top Panel
    uint32_t txt_id;
    BOS_CreateTextbox(s_demo_win_id, 10, 70, 200, 30, "Type something...", &txt_id);
    BWE_Window* t_win = BWE_GetWindow(txt_id);
    if (t_win) {
        t_win->anchor_flags = BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP;
    }

    // 6. Create Progress Bar
    BOS_CreateProgressBar(s_demo_win_id, 10, 110, 200, 20, 0, 100, &s_progress_id);
    BWE_Window* pr_win = BWE_GetWindow(s_progress_id);
    if (pr_win) {
        pr_win->anchor_flags = BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_RIGHT;
        pr_win->control_data.progressbar.value = 30; // 30% initial progress
    }

    // 7. Create ListView on left bottom
    uint32_t lv_id;
    BOS_CreateListView(s_demo_win_id, 10, 140, 180, 120, &lv_id);
    BWE_Window* lv_win = BWE_GetWindow(lv_id);
    if (lv_win) {
        lv_win->anchor_flags = BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_BOTTOM;
        
        BOS_ListView_AddItem(lv_id, "Standard File");
        BOS_ListView_AddItem(lv_id, "Visual Designer");
        BOS_ListView_AddItem(lv_id, "Terminal Setup");
        BOS_ListView_AddItem(lv_id, "OS Kernel Config");
    }

    // 8. Create TreeView in middle bottom
    uint32_t tv_id;
    BOS_CreateTreeView(s_demo_win_id, 200, 140, 180, 120, &tv_id);
    BWE_Window* tv_win = BWE_GetWindow(tv_id);
    if (tv_win) {
        tv_win->anchor_flags = BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_BOTTOM;
        
        int32_t r_idx, c_idx;
        BOS_TreeView_AddNode(tv_id, "Root Node", -1, &r_idx);
        BOS_TreeView_AddNode(tv_id, "Child branch A", r_idx, &c_idx);
        BOS_TreeView_AddNode(tv_id, "Leaf Node A1", c_idx, 0);
        BOS_TreeView_AddNode(tv_id, "Child branch B", r_idx, &c_idx);
        BOS_TreeView_AddNode(tv_id, "Leaf Node B1", c_idx, 0);
    }

    // 9. Create ScrollBar on right bottom
    uint32_t sb_id;
    BOS_CreateScrollBar(s_demo_win_id, 390, 140, 20, 120, true, 0, 100, 0, &sb_id);
    BWE_Window* sb_win = BWE_GetWindow(sb_id);
    if (sb_win) {
        sb_win->anchor_flags = BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_BOTTOM;
    }

    // 10. Create Custom Drawing Canvas
    uint32_t cv_id;
    BOS_CreateCanvas(s_demo_win_id, 420, 140, 150, 120, on_canvas_custom_paint, &cv_id);
    BWE_Window* cv_win = BWE_GetWindow(cv_id);
    if (cv_win) {
        cv_win->anchor_flags = BWE_ANCHOR_ALL;
    }

    // Trigger parent docking calculation
    BWE_UpdateLayout(s_demo_win_id);
    
    // Present main container
    BOS_Show(s_demo_win_id);
    BOS_SetFocus(s_demo_win_id);
    
    display_print("[DEMO_APP] Main Demo Window and controls initialized successfully\n");
}
