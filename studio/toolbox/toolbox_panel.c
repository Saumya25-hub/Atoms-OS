#include "studio/include/studio_toolbox.h"
#include "kernel/core/lib/include/string.h"

static BOS_ToolboxItem g_toolbox_items[] = {
    { "Containers", "Grid", "icon_grid" },
    { "Containers", "StackPanel", "icon_stack" },
    { "Containers", "DockPanel", "icon_dock" },
    { "Controls", "Button", "icon_btn" },
    { "Controls", "Label", "icon_lbl" },
    { "Controls", "TextBox", "icon_txt" },
    { "Controls", "CheckBox", "icon_chk" },
    { "Controls", "ComboBox", "icon_combo" },
    { "Controls", "ListView", "icon_list" },
    { "Controls", "ProgressBar", "icon_progress" }
};

void BOS_Studio_ToolboxInit(void) {
    /* Initialize palette */
}

uint32_t BOS_Studio_ToolboxGetCount(void) {
    return sizeof(g_toolbox_items) / sizeof(BOS_ToolboxItem);
}

BOS_ToolboxItem BOS_Studio_ToolboxGetItem(uint32_t index) {
    if (index >= BOS_Studio_ToolboxGetCount()) {
        BOS_ToolboxItem dummy = { "", "", "" };
        return dummy;
    }
    return g_toolbox_items[index];
}
