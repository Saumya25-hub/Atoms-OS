#include "explorer_sidebar.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/botheme/botheme.h"
#include "kernel/engine/horse_engine.h"

static void sidebar_btn_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    
    const char* text = btn->control_data.button.text;
    if (strstr(text, "Home") || strstr(text, "This PC")) explorer_navigate(ctx, "/");
    else if (strstr(text, "Desktop")) explorer_navigate(ctx, "/DESKTOP");
    else if (strstr(text, "Documents")) explorer_navigate(ctx, "/DOCS");
    else if (strstr(text, "Downloads")) explorer_navigate(ctx, "/DOWNLOAD");
    else if (strstr(text, "Music")) explorer_navigate(ctx, "/MUSIC");
    else if (strstr(text, "Gallery") || strstr(text, "Pictures")) explorer_navigate(ctx, "/PHOTO");
    else if (strstr(text, "Videos")) explorer_navigate(ctx, "/VIDEO");
    else if (strstr(text, "USB Drive") || strstr(text, "U:")) explorer_navigate(ctx, "U:\\");
    else if (strstr(text, "Settings")) {
        horse_launch(APP_ID_SETTINGS);
    }
}

void explorer_sidebar_create(ExplorerContext* ctx) {
    if (!ctx || ctx->sidebar_id == 0) return;
    
    uint32_t y = 10;
    uint32_t tmp;
    
    const char* items[] = {
        "  Home",
        "  Desktop",
        "  Documents",
        "  Downloads",
        "  Gallery",
        "  Music",
        "  Videos",
        "  This PC",
        "  Network",
        "  Settings",
        "  Recycle Bin"
    };
    
    for (int i = 0; i < 11; i++) {
        BOS_CreateButton(ctx->sidebar_id, 8, y, 160, 26, items[i], sidebar_btn_clicked, &tmp);
        BWE_Window* btn = BWE_GetWindow(tmp);
        if (btn) {
            btn->control_data.button.bg_color = 0xEE1E293B; // Dark Slate acrylic
            btn->control_data.button.text_color = 0xFFF1F5F9; // Pure white text
        }
        y += 30;
    }
}
