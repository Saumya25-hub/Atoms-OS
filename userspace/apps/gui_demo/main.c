#include "../../libbos/include/bos.h"
#include "../../libbos_gui/include/bos_gui.h"

static BOSLabel* status_label = 0;
static int click_count = 0;

static BOSProgressBar* progress_bar = 0;
static BOSCheckBox* checkbox = 0;

void on_checkbox_toggle(bool checked) {
    if (checked) {
        bos_print("Checkbox is CHECKED!\n");
    } else {
        bos_print("Checkbox is UNCHECKED!\n");
    }
}

static void bos_print_dec(int val) {
    if (val == 0) { bos_print("0"); return; }
    char buf[16];
    int i = 14;
    buf[15] = '\0';
    while (val > 0 && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    bos_print(&buf[i + 1]);
}

void on_button_click(void) {
    click_count++;
    
    if (progress_bar) {
        BOS_ProgressBarSetValue(progress_bar, click_count * 10);
    }
    
    bos_print("Button clicked! Count: ");
    bos_print_dec(click_count);
    bos_print("\n");
}

void main(void) {
    bos_print("Starting GUI Demo Application...\n");
    BOS_GUI_Init();

    BOSWindow* window = BOS_CreateWindow("GUI Demo", 100, 100, 400, 300);
    if (!window) {
        bos_print("Failed to create window!\n");
        bos_exit();
    }

    BOSPanel* panel = BOS_CreatePanel(window, 10, 30, 380, 260, 0xFF1E293B); // Dark panel
    BOS_PanelSetCornerRadius(panel, 12);
    BOS_PanelSetGradient(panel, 0xFF1E293B, 0xFF0F172A, BOS_GRADIENT_VERTICAL);
    
    BOSLabel* label = BOS_CreateLabel(window, "Welcome to BOS Native SDK", 20, 50, 0xFFFFFFFF);
    
    BOSButton* btn = BOS_CreateButtonInPanel(panel, "Step Forward", 20, 80, 120, 40, on_button_click);
    BOS_ButtonSetCornerRadius(btn, 8);
    BOS_ButtonSetGradient(btn, 0xFF3B82F6, 0xFF1D4ED8, BOS_GRADIENT_VERTICAL);
    
    checkbox = BOS_CreateCheckBox(window, "Enable Advanced Mode", 150, 80, false, on_checkbox_toggle);
    
    BOS_CreateLabel(window, "Progress:", 20, 140, 0xFFFFFFFF);
    progress_bar = BOS_CreateProgressBar(window, 20, 160, 340, 20);

    BOS_ShowWindow(window);
    
    bos_print("Window shown. Entering event loop...\n");
    BOS_Run();
    
    bos_exit();
}

void _start(void) {
    main();
    bos_exit();
}
