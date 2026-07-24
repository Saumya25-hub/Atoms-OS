#include "app_window_api.h"
#include "kernel/application/app_manager/app_manager.h"
#include "kernel/wm/bwe/include/bwe.h"
#include <stddef.h>

bwe_error_t ATOMS_CreateWindowForApp(uint32_t app_id, int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_win_id) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return BWE0001;

    uint32_t win_id = 0;
    bwe_error_t err = BOS_CreateWindow(x, y, width, height, title, &win_id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (win) {
        win->owner_pid = app->pid;
    }

    if (app->main_window_id == 0) {
        app->main_window_id = win_id;
    } else if (app->window_count < ATOMS_MAX_WINDOWS_PER_APP) {
        app->window_ids[app->window_count++] = win_id;
    }

    if (out_win_id) *out_win_id = win_id;
    return BWE_SUCCESS;
}

bwe_error_t ATOMS_CloseWindowForApp(uint32_t app_id, uint32_t win_id) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return BWE0001;

    bwe_error_t err = BOS_DestroySurface(win_id);

    if (app->main_window_id == win_id) {
        app->main_window_id = 0;
    }
    for (uint32_t i = 0; i < app->window_count; i++) {
        if (app->window_ids[i] == win_id) {
            for (uint32_t j = i; j < app->window_count - 1; j++) {
                app->window_ids[j] = app->window_ids[j + 1];
            }
            app->window_count--;
            break;
        }
    }

    return err;
}

bwe_error_t ATOMS_SetWindowBounds(uint32_t win_id, int32_t x, int32_t y, int32_t width, int32_t height) {
    return BOS_SetBounds(win_id, (uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height);
}

bwe_error_t ATOMS_SetWindowTitle(uint32_t win_id, const char* title) {
    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return BWE0001;

    uint32_t i = 0;
    while (title[i] != '\0' && i < sizeof(win->title) - 1) {
        win->title[i] = title[i];
        i++;
    }
    win->title[i] = '\0';
    win->is_dirty = true;
    return BWE_SUCCESS;
}
