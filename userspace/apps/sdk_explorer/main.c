#include "../../libbos/include/bos.h"
#include "../../libbos_gui/include/bos_gui.h"

// ============================================================
// SDK Explorer State & Global Controls
// ============================================================
static BOSWindow* main_window = 0;
static BOSPanel* left_nav_panel = 0;
static BOSPanel* preview_area_panel = 0;
static BOSPanel* status_bar_panel = 0;
static BOSLabel* status_label = 0;

// 12 Page Panels
static BOSPanel* page_panels[12] = {0};
static const char* page_names[12] = {
    "Buttons", "Labels", "TextBox", "CheckBox", "Radio Buttons",
    "Progress Bar", "Panels", "Gradient", "Rounded Corners",
    "Scroll Viewer", "Layout", "Window"
};
static int current_page_idx = 0;

// Interactive state variables
static int g_click_count = 0;
static BOSLabel* g_click_counter_label = 0;
static BOSProgressBar* g_progress_bar = 0;
static int g_progress_val = 40;

// Helper to switch pages
static void switch_page(int page_idx) {
    if (page_idx < 0 || page_idx >= 12) return;
    current_page_idx = page_idx;
    
    for (int i = 0; i < 12; i++) {
        if (page_panels[i]) {
            if (i == page_idx) {
                BOS_SetBounds(page_panels[i]->id, 0, 0, 560, 420);
            } else {
                BOS_SetBounds(page_panels[i]->id, -2000, -2000, 560, 420); // Hide offscreen
            }
        }
    }
    
    if (status_label) {
        char status_msg[128] = "Page: ";
        int pos = 6;
        const char* name = page_names[page_idx];
        for (int k = 0; name[k] && pos < 100; k++) status_msg[pos++] = name[k];
        const char* suffix = " | Status: Active | SDK Explorer v1.0";
        for (int k = 0; suffix[k] && pos < 120; k++) status_msg[pos++] = suffix[k];
        status_msg[pos] = '\0';
        BOS_SetText(status_label->id, status_msg);
    }
}

// Navigation Callbacks
static void nav_cb_0(void) { switch_page(0); }
static void nav_cb_1(void) { switch_page(1); }
static void nav_cb_2(void) { switch_page(2); }
static void nav_cb_3(void) { switch_page(3); }
static void nav_cb_4(void) { switch_page(4); }
static void nav_cb_5(void) { switch_page(5); }
static void nav_cb_6(void) { switch_page(6); }
static void nav_cb_7(void) { switch_page(7); }
static void nav_cb_8(void) { switch_page(8); }
static void nav_cb_9(void) { switch_page(9); }
static void nav_cb_10(void) { switch_page(10); }
static void nav_cb_11(void) { switch_page(11); }

static void (*nav_callbacks[12])(void) = {
    nav_cb_0, nav_cb_1, nav_cb_2, nav_cb_3, nav_cb_4, nav_cb_5,
    nav_cb_6, nav_cb_7, nav_cb_8, nav_cb_9, nav_cb_10, nav_cb_11
};

// Button Page Callbacks
static void on_test_button_click(void) {
    g_click_count++;
    char buf[64] = "Click Counter: ";
    int val = g_click_count;
    char num[16]; int i = 0;
    if (val == 0) num[i++] = '0';
    while (val > 0) { num[i++] = '0' + (val % 10); val /= 10; }
    int p = 15;
    for (int j = i - 1; j >= 0; j--) buf[p++] = num[j];
    buf[p] = '\0';
    if (g_click_counter_label) {
        BOS_SetText(g_click_counter_label->id, buf);
    }
}

// Progress Bar Callbacks
static void on_pb_inc(void) {
    g_progress_val += 10;
    if (g_progress_val > 100) g_progress_val = 100;
    if (g_progress_bar) BOS_ProgressBarSetValue(g_progress_bar, g_progress_val);
}

static void on_pb_dec(void) {
    g_progress_val -= 10;
    if (g_progress_val < 0) g_progress_val = 0;
    if (g_progress_bar) BOS_ProgressBarSetValue(g_progress_bar, g_progress_val);
}

// Radio Button Callback
static void on_radio_selected(uint32_t group_id) {
    (void)group_id;
    bos_print("Radio Button selected!\n");
}

// CheckBox Callback
static void on_checkbox_toggled(bool checked) {
    bos_print("CheckBox state toggled\n");
}

// ============================================================
// Page Setup Renders
// ============================================================

static void setup_buttons_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "1. Buttons Showcase", 15, 15, 0xFF60A5FA);
    BOS_CreateLabelInPanel(pnl, "Tests different button sizes, rounded corners, and gradients.", 15, 35, 0xFF94A3B8);

    BOSButton* btn1 = BOS_CreateButtonInPanel(pnl, "Standard Button", 15, 70, 140, 35, on_test_button_click);
    (void)btn1;
    
    BOSButton* btn2 = BOS_CreateButtonInPanel(pnl, "Rounded 10px", 170, 70, 140, 35, on_test_button_click);
    BOS_ButtonSetCornerRadius(btn2, 10);

    BOSButton* btn3 = BOS_CreateButtonInPanel(pnl, "Gradient Pill", 325, 70, 140, 35, on_test_button_click);
    BOS_ButtonSetCornerRadius(btn3, 17);
    BOS_ButtonSetGradient(btn3, 0xFF3B82F6, 0xFF1D4ED8, BOS_GRADIENT_VERTICAL);

    BOSButton* btn4 = BOS_CreateButtonInPanel(pnl, "Large Action", 15, 125, 200, 45, on_test_button_click);
    BOS_ButtonSetCornerRadius(btn4, 8);
    BOS_ButtonSetGradient(btn4, 0xFF10B981, 0xFF047857, BOS_GRADIENT_HORIZONTAL);

    g_click_counter_label = BOS_CreateLabelInPanel(pnl, "Click Counter: 0", 230, 140, 0xFFF59E0B);
}

static void setup_labels_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "2. Labels & Typography Showcase", 15, 15, 0xFF60A5FA);
    BOS_CreateLabelInPanel(pnl, "Standard White Label", 15, 50, 0xFFFFFFFF);
    BOS_CreateLabelInPanel(pnl, "Emerald Green Status Text", 15, 80, 0xFF34D399);
    BOS_CreateLabelInPanel(pnl, "Amber Warning Notification Label", 15, 110, 0xFFFBBF24);
    BOS_CreateLabelInPanel(pnl, "Rose Red Error Diagnostics Label", 15, 140, 0xFFF87171);
    BOS_CreateLabelInPanel(pnl, "BOS OS — Engineered for pure performance and ultra low latency UI.", 15, 180, 0xFFCBD5E1);
}

static void setup_textbox_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "3. TextBox Input Showcase", 15, 15, 0xFF60A5FA);
    
    BOS_CreateLabelInPanel(pnl, "Username:", 15, 55, 0xFFE2E8F0);
    BOS_CreateTextBoxInPanel(pnl, "developer_user", 120, 50, 250, 32);

    BOS_CreateLabelInPanel(pnl, "Password:", 15, 105, 0xFFE2E8F0);
    BOS_CreateTextBoxInPanel(pnl, "********", 120, 100, 250, 32);

    BOS_CreateLabelInPanel(pnl, "System Prompt:", 15, 155, 0xFFE2E8F0);
    BOS_CreateTextBoxInPanel(pnl, "Enter high-level BOS SDK declarations...", 120, 150, 380, 32);
}

static void setup_checkbox_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "4. CheckBox Showcase", 15, 15, 0xFF60A5FA);
    BOS_CreateCheckBoxInPanel(pnl, "Enable Hardware Acceleration", 15, 60, true, on_checkbox_toggled);
    BOS_CreateCheckBoxInPanel(pnl, "Enable Occlusion Culling (60 FPS)", 15, 100, true, on_checkbox_toggled);
    BOS_CreateCheckBoxInPanel(pnl, "Enable Debug Telemetry Overlay", 15, 140, false, on_checkbox_toggled);
    BOS_CreateCheckBoxInPanel(pnl, "Strict Ring-0 Syscall Isolation", 15, 180, true, on_checkbox_toggled);
}

static void setup_radio_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "5. Radio Button Group Showcase", 15, 15, 0xFF60A5FA);
    BOS_CreateLabelInPanel(pnl, "Select Graphics Backend:", 15, 45, 0xFF94A3B8);
    
    BOS_CreateRadioButtonInPanel(pnl, "BSPE Bochs VBE Framebuffer", 20, 75, 1, true, on_radio_selected);
    BOS_CreateRadioButtonInPanel(pnl, "BOGE Direct Hardware Surface", 20, 110, 1, false, on_radio_selected);
    BOS_CreateRadioButtonInPanel(pnl, "Software Fallback Memory Pipeline", 20, 145, 1, false, on_radio_selected);
}

static void setup_progress_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "6. Progress Bar Showcase", 15, 15, 0xFF60A5FA);
    
    g_progress_bar = BOS_CreateProgressBarInPanel(pnl, 15, 60, 400, 24);
    BOS_ProgressBarSetValue(g_progress_bar, g_progress_val);

    BOSButton* btn_dec = BOS_CreateButtonInPanel(pnl, "- 10%", 15, 110, 90, 35, on_pb_dec);
    BOS_ButtonSetCornerRadius(btn_dec, 6);
    
    BOSButton* btn_inc = BOS_CreateButtonInPanel(pnl, "+ 10%", 120, 110, 90, 35, on_pb_inc);
    BOS_ButtonSetCornerRadius(btn_inc, 6);
    BOS_ButtonSetGradient(btn_inc, 0xFF10B981, 0xFF047857, BOS_GRADIENT_VERTICAL);
}

static void setup_panels_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "7. Panels & Containers Showcase", 15, 15, 0xFF60A5FA);
    
    BOSPanel* solid_panel = BOS_CreatePanelInPanel(pnl, 15, 45, 230, 150, 0xFF1E293B);
    BOS_CreateLabelInPanel(solid_panel, "Solid Panel (0xFF1E293B)", 10, 10, 0xFF94A3B8);

    BOSPanel* rounded_panel = BOS_CreatePanelInPanel(pnl, 260, 45, 230, 150, 0xFF0F172A);
    BOS_PanelSetCornerRadius(rounded_panel, 14);
    BOS_CreateLabelInPanel(rounded_panel, "Rounded Panel (14px)", 10, 10, 0xFF38BDF8);

    BOSPanel* nested = BOS_CreatePanelInPanel(rounded_panel, 15, 45, 200, 80, 0xFF1E293B);
    BOS_PanelSetCornerRadius(nested, 8);
    BOS_CreateLabelInPanel(nested, "Nested Panel inside Panel", 10, 10, 0xFFF1F5F9);
}

static void setup_gradient_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "8. Linear Gradient Background Showcase", 15, 15, 0xFF60A5FA);

    BOSPanel* p1 = BOS_CreatePanelInPanel(pnl, 15, 45, 150, 180, 0xFF1E293B);
    BOS_PanelSetCornerRadius(p1, 8);
    BOS_CreateLabelInPanel(p1, "Solid Fill", 10, 10, 0xFFFFFFFF);

    BOSPanel* p2 = BOS_CreatePanelInPanel(pnl, 180, 45, 150, 180, 0xFF3B82F6);
    BOS_PanelSetCornerRadius(p2, 8);
    BOS_PanelSetGradient(p2, 0xFF3B82F6, 0xFF1E1B4B, BOS_GRADIENT_VERTICAL);
    BOS_CreateLabelInPanel(p2, "Vertical Gradient", 10, 10, 0xFFFFFFFF);

    BOSPanel* p3 = BOS_CreatePanelInPanel(pnl, 345, 45, 150, 180, 0xFF10B981);
    BOS_PanelSetCornerRadius(p3, 8);
    BOS_PanelSetGradient(p3, 0xFF10B981, 0xFF064E3B, BOS_GRADIENT_HORIZONTAL);
    BOS_CreateLabelInPanel(p3, "Horizontal Gradient", 10, 10, 0xFFFFFFFF);
}

static void setup_rounded_corners_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "9. Corner Radius Visual Matrix", 15, 15, 0xFF60A5FA);

    int radii[6] = {0, 4, 8, 12, 20, 30};
    int x_off = 15;
    int y_off = 45;

    for (int k = 0; k < 6; k++) {
        BOSPanel* card = BOS_CreatePanelInPanel(pnl, x_off, y_off, 140, 75, 0xFF1E293B);
        if (radii[k] > 0) BOS_PanelSetCornerRadius(card, radii[k]);
        
        char label_buf[32] = "Radius: ";
        int pos = 8;
        int r = radii[k];
        if (r == 0) label_buf[pos++] = '0';
        else {
            if (r >= 10) label_buf[pos++] = '0' + (r / 10);
            label_buf[pos++] = '0' + (r % 10);
        }
        label_buf[pos++] = 'p'; label_buf[pos++] = 'x'; label_buf[pos] = '\0';
        
        BOS_CreateLabelInPanel(card, label_buf, 10, 10, 0xFF38BDF8);

        x_off += 155;
        if (k == 2) { x_off = 15; y_off += 95; }
    }
}

static void setup_scroll_viewer_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "10. Scroll Viewer Showcase", 15, 15, 0xFF60A5FA);
    
    BOSScrollViewer* sv = BOS_CreateScrollViewerInPanel(pnl, 15, 45, 480, 220, 600);
    if (sv) {
        BOSPanel* content = (BOSPanel*)&sv->content_panel_id; // Panel reference
        for (int i = 0; i < 15; i++) {
            char line_buf[64] = "Scroll Content Row #";
            int p = 20;
            int num = i + 1;
            if (num >= 10) line_buf[p++] = '0' + (num / 10);
            line_buf[p++] = '0' + (num % 10);
            line_buf[p] = '\0';
            BOS_CreateLabelInPanel(content, line_buf, 15, 10 + (i * 35), 0xFFE2E8F0);
        }
    }
}

static void setup_layout_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "11. Layout Engines (Dock, Stack, Grid)", 15, 15, 0xFF60A5FA);

    BOSPanel* stack_demo = BOS_CreatePanelInPanel(pnl, 15, 45, 220, 180, 0xFF1E293B);
    BOS_PanelSetCornerRadius(stack_demo, 8);
    BOS_CreateLabelInPanel(stack_demo, "Vertical Stack Layout", 10, 10, 0xFFF59E0B);
    
    BOSButton* b1 = BOS_CreateButtonInPanel(stack_demo, "Item 1", 10, 35, 200, 30, 0);
    BOSButton* b2 = BOS_CreateButtonInPanel(stack_demo, "Item 2", 10, 70, 200, 30, 0);
    BOSButton* b3 = BOS_CreateButtonInPanel(stack_demo, "Item 3", 10, 105, 200, 30, 0);
    (void)b1; (void)b2; (void)b3;

    BOSPanel* grid_demo = BOS_CreatePanelInPanel(pnl, 255, 45, 220, 180, 0xFF0F172A);
    BOS_PanelSetCornerRadius(grid_demo, 8);
    BOS_CreateLabelInPanel(grid_demo, "Flex Grid Layout", 10, 10, 0xFF10B981);
    
    BOSPanel* g1 = BOS_CreatePanelInPanel(grid_demo, 10, 35, 95, 60, 0xFF334155);
    BOSPanel* g2 = BOS_CreatePanelInPanel(grid_demo, 115, 35, 95, 60, 0xFF334155);
    BOSPanel* g3 = BOS_CreatePanelInPanel(grid_demo, 10, 105, 200, 60, 0xFF1E293B);
    (void)g1; (void)g2; (void)g3;
}

static void setup_window_page(BOSPanel* pnl) {
    BOS_CreateLabelInPanel(pnl, "12. Window Properties Showcase", 15, 15, 0xFF60A5FA);
    BOS_CreateLabelInPanel(pnl, "Window Title: 'SDK Explorer — BOS Native Control Showcase'", 15, 50, 0xFFE2E8F0);
    BOS_CreateLabelInPanel(pnl, "Window Geometry: 760 x 520 px (Absolute Screen Space)", 15, 80, 0xFFE2E8F0);
    BOS_CreateLabelInPanel(pnl, "Supported Actions: Move, Resize, Minimize, Maximize, Close", 15, 110, 0xFF34D399);
    BOS_CreateLabelInPanel(pnl, "Compositing Engine: BOGE V2 Decoupled 60 FPS Scanout", 15, 140, 0xFFF59E0B);
}

// ============================================================
// Main Application Entry Point
// ============================================================

void main(void) {
    bos_print("Launching BOS SDK Explorer...\n");
    BOS_GUI_Init();

    // 1. Create Main Window
    main_window = BOS_CreateWindow("SDK Explorer — BOS Native Control Showcase", 50, 40, 760, 520);
    if (!main_window) {
        bos_print("Error: Failed to create main SDK Explorer window!\n");
        bos_exit();
    }

    // 2. Left Navigation Panel (160px wide)
    left_nav_panel = BOS_CreatePanel(main_window, 10, 35, 160, 435, 0xFF0F172A);
    BOS_PanelSetCornerRadius(left_nav_panel, 8);

    BOS_CreateLabelInPanel(left_nav_panel, "SDK CONTROLS", 12, 12, 0xFF94A3B8);

    int btn_y = 35;
    for (int i = 0; i < 12; i++) {
        BOSButton* nav_btn = BOS_CreateButtonInPanel(left_nav_panel, page_names[i], 8, btn_y, 144, 28, nav_callbacks[i]);
        BOS_ButtonSetCornerRadius(nav_btn, 4);
        btn_y += 32;
    }

    // 3. Right Preview Area Panel (570px wide)
    preview_area_panel = BOS_CreatePanel(main_window, 178, 35, 570, 435, 0xFF1E293B);
    BOS_PanelSetCornerRadius(preview_area_panel, 8);

    // Create 12 Page Panels inside Preview Area
    for (int i = 0; i < 12; i++) {
        page_panels[i] = BOS_CreatePanelInPanel(preview_area_panel, -2000, -2000, 560, 420, 0xFF1E293B);
        BOS_PanelSetCornerRadius(page_panels[i], 8);
    }

    // Initialize all showcase pages
    setup_buttons_page(page_panels[0]);
    setup_labels_page(page_panels[1]);
    setup_textbox_page(page_panels[2]);
    setup_checkbox_page(page_panels[3]);
    setup_radio_page(page_panels[4]);
    setup_progress_page(page_panels[5]);
    setup_panels_page(page_panels[6]);
    setup_gradient_page(page_panels[7]);
    setup_rounded_corners_page(page_panels[8]);
    setup_scroll_viewer_page(page_panels[9]);
    setup_layout_page(page_panels[10]);
    setup_window_page(page_panels[11]);

    // 4. Bottom Status Bar Panel
    status_bar_panel = BOS_CreatePanel(main_window, 10, 475, 738, 30, 0xFF0F172A);
    BOS_PanelSetCornerRadius(status_bar_panel, 4);
    status_label = BOS_CreateLabelInPanel(status_bar_panel, "Page: Buttons | Status: Active | SDK Explorer v1.0", 12, 8, 0xFF64748B);

    // Show initial page (Buttons)
    switch_page(0);

    // Show Window and enter event loop
    BOS_ShowWindow(main_window);
    bos_print("SDK Explorer initialized. Entering event loop...\n");
    BOS_Run();

    bos_exit();
}

void _start(void) {
    main();
    bos_exit();
}
