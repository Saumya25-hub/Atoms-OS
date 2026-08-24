#include "wallpaper_settings.h"
#include "wallpaper_manager.h"
#include "kernel/services/wallpaper/wallpaper_service.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/gui/controls/control.h"
#include "kernel/gui/controls/button.h"
#include "kernel/gui/controls/label.h"
#include "kernel/gui/controls/panel.h"
#include "kernel/core/lib/include/string.h"

static void btn_wallpaper_click(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    uint32_t wid = (uint32_t)(uintptr_t)btn->user_data;
    wallpaper_service_set_index(wid);
}

static void btn_next_wallpaper_click(uint32_t btn_id) {
    (void)btn_id;
    wallpaper_service_select_next();
}

void wallpaper_settings_render(uint32_t panel_id) {
    uint32_t dummy = 0;
    BOS_CreateLabel(panel_id, 15, 12, "Desktop Personalization", 0xFF0F172A, &dummy);
    BOS_CreateLabel(panel_id, 15, 34, "Select an embedded 1080p wallpaper:", 0xFF475569, &dummy);
    
    const char* wp_names[] = {
        "1. Cyberpunk Neon (W1)",
        "2. Deep Blue Horizon (W2)",
        "3. Abstract Aurora (W3)"
    };
    uint32_t count = wallpaper_service_get_count();
    if (count > 3) count = 3;
    
    uint32_t current_id = wallpaper_service_get_selected_id();

    int y_pos = 62;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t btn_id = 0;
        char label[64];
        if (i == current_id) {
            strcpy(label, "* ");
            strcat(label, wp_names[i]);
            strcat(label, " [Active]");
        } else {
            strcpy(label, wp_names[i]);
        }
        BOS_CreateButton(panel_id, 15, y_pos, 250, 36, label, btn_wallpaper_click, &btn_id);
        
        BWE_Window* btn = BWE_GetWindow(btn_id);
        if (btn) {
            btn->user_data = (void*)(uintptr_t)i;
        }
        
        y_pos += 44;
    }

    uint32_t next_btn_id = 0;
    BOS_CreateButton(panel_id, 15, y_pos + 6, 250, 36, ">> Next Wallpaper in Line", btn_next_wallpaper_click, &next_btn_id);
}
