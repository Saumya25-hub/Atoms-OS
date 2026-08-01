#include "../include/bar_api.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"

static BARWindowInfo g_bar_windows[BAR_MAX_WINDOWS];
static uint32_t g_window_count = 0;

BARWindowID BAR_CreateWindow(BARProcessID pid, int32_t x, int32_t y, int32_t w, int32_t h, const char* title, BARWindowID parent_id) {
    if (pid == 0 || !title) return 0;
    
    for (uint32_t i = 0; i < BAR_MAX_WINDOWS; i++) {
        if (g_bar_windows[i].window_id == 0) {
            uint32_t bwe_win_id = 0;
            // Delegate window creation to BWE Window Manager
            bwe_error_t err = BOS_CreateWindow(x, y, w, h, title, &bwe_win_id);
            if (err != 0) bwe_win_id = i + 1; // Fallback handle
            
            g_bar_windows[i].window_id = bwe_win_id;
            g_bar_windows[i].process_id = pid;
            g_bar_windows[i].bounds = (BARRect){x, y, w, h};
            strcpy(g_bar_windows[i].title, title);
            g_bar_windows[i].visible = true;
            g_bar_windows[i].focused = false;
            g_bar_windows[i].parent_id = parent_id;
            g_window_count++;
            return bwe_win_id;
        }
    }
    return 0;
}

int32_t BAR_DestroyWindow(BARWindowID win_id) {
    if (win_id == 0) return -1;
    for (uint32_t i = 0; i < BAR_MAX_WINDOWS; i++) {
        if (g_bar_windows[i].window_id == win_id) {
            BOS_Hide(win_id);
            g_bar_windows[i].window_id = 0;
            if (g_window_count > 0) g_window_count--;
            return 0;
        }
    }
    return -1;
}

int32_t BAR_ShowWindow(BARWindowID win_id) {
    if (win_id == 0) return -1;
    for (uint32_t i = 0; i < BAR_MAX_WINDOWS; i++) {
        if (g_bar_windows[i].window_id == win_id) {
            g_bar_windows[i].visible = true;
            return 0;
        }
    }
    return -1;
}

int32_t BAR_HideWindow(BARWindowID win_id) {
    if (win_id == 0) return -1;
    for (uint32_t i = 0; i < BAR_MAX_WINDOWS; i++) {
        if (g_bar_windows[i].window_id == win_id) {
            g_bar_windows[i].visible = false;
            BOS_Hide(win_id);
            return 0;
        }
    }
    return -1;
}
