#include "wallpaper_settings.h"
#include "wallpaper_manager.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/gui/controls/control.h"
#include "kernel/gui/controls/button.h"
#include "kernel/gui/controls/label.h"
#include "kernel/gui/controls/panel.h"

static void btn_wallpaper_click(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    uint32_t wid = (uint32_t)(uintptr_t)btn->user_data;
    wallpaper_set(wid);
}

void wallpaper_settings_render(uint32_t panel_id) {
    uint32_t dummy = 0;
    BOS_CreateLabel(panel_id, 15, 15, "Desktop Personalization", 0xFF0F172A, &dummy);
    BOS_CreateLabel(panel_id, 15, 45, "Select a wallpaper to instantly apply it.", 0xFF475569, &dummy);
    
    WallpaperEntry* all = wallpaper_registry_get_all();
    int count = wallpaper_registry_get_count();
    
    int y_pos = 80;
    for (int i = 0; i < count; i++) {
        uint32_t btn_id = 0;
        BOS_CreateButton(panel_id, 15, y_pos, 200, 35, all[i].name, btn_wallpaper_click, &btn_id);
        
        BWE_Window* btn = BWE_GetWindow(btn_id);
        if (btn) {
            btn->user_data = (void*)(uintptr_t)all[i].id;
        }
        
        y_pos += 45;
    }
}
