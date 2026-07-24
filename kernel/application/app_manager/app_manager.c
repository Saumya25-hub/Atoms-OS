// ============================================================
// ATOMS OS — Phase 9 Application Manager Framework
// ============================================================

#include "app_manager.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static ATOMS_Application app_pool[ATOMS_MAX_APPS];
static uint32_t next_app_id = 1;
static uint32_t next_pid = 100;
static uint32_t g_focused_app_id = 0;

static void app_log(const char* level, const char* msg) {
    display_print("[APP_MGR:");
    display_print(level);
    display_print("] ");
    display_print(msg);
    display_print("\n");
}

static void app_log_id(const char* level, const char* msg, uint32_t id) {
    display_print("[APP_MGR:");
    display_print(level);
    display_print("] ");
    display_print(msg);
    display_print(" #");
    display_print_dec(id);
    display_print("\n");
}

void ATOMS_AppManager_Init(void) {
    for (uint32_t i = 0; i < ATOMS_MAX_APPS; i++) {
        app_pool[i].app_id = 0;
        app_pool[i].state = ATOMS_APP_STATE_CLOSED;
        app_pool[i].pid = 0;
        app_pool[i].main_window_id = 0;
        app_pool[i].window_count = 0;
        app_pool[i].capabilities_mask = 0;
        app_pool[i].is_builtin = true;
        app_pool[i].on_init = 0;
        app_pool[i].on_exit = 0;
        app_pool[i].name[0] = '\0';
        app_pool[i].version[0] = '\0';
        app_pool[i].executable_path[0] = '\0';
        for (uint32_t w = 0; w < 16; w++) app_pool[i].window_ids[w] = 0;
    }
    next_app_id = 1;
    next_pid = 100;
    g_focused_app_id = 0;
    app_log("INFO", "ATOMS Native Application Framework Manager Initialized");
}

bwe_error_t ATOMS_RegisterApplication(const char* name, const char* version, const char* exec_path,
                                       ATOMS_AppInitFunc on_init, ATOMS_AppExitFunc on_exit,
                                       uint32_t capabilities_mask, uint32_t* out_app_id) {
    if (!name) return BWE0001;

    ATOMS_Application* app = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_APPS; i++) {
        if (app_pool[i].state == ATOMS_APP_STATE_CLOSED && app_pool[i].app_id == 0) {
            app = &app_pool[i];
            break;
        }
    }
    if (!app) return BWE0004;

    app->app_id = next_app_id++;
    strncpy(app->name, name, 63); app->name[63] = '\0';
    strncpy(app->version, version ? version : "1.0", 15); app->version[15] = '\0';
    if (exec_path) {
        strncpy(app->executable_path, exec_path, 127);
        app->executable_path[127] = '\0';
        app->is_builtin = false;
    } else {
        app->executable_path[0] = '\0';
        app->is_builtin = true;
    }

    app->on_init = on_init;
    app->on_exit = on_exit;
    app->capabilities_mask = capabilities_mask;
    app->state = ATOMS_APP_STATE_CLOSED;
    app->pid = 0;
    app->main_window_id = 0;
    app->window_count = 0;

    if (out_app_id) *out_app_id = app->app_id;

    app_log("INFO", "Registered Application:");
    display_print("  Name: "); display_print(app->name);
    display_print(" Version: "); display_print(app->version); display_print("\n");

    return BWE_SUCCESS;
}

bwe_error_t ATOMS_StartApplication(uint32_t app_id) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return BWE0001;

    if (app->state == ATOMS_APP_STATE_RUNNING || app->state == ATOMS_APP_STATE_BACKGROUND) {
        app_log_id("INFO", "App already running, focusing main window", app->main_window_id);
        if (app->main_window_id) BOS_SetFocus(app->main_window_id);
        app->state = ATOMS_APP_STATE_RUNNING;
        g_focused_app_id = app->app_id;
        return BWE_SUCCESS;
    }

    app_log("INFO", "Starting Application:");
    display_print("  Name: "); display_print(app->name); display_print("\n");

    app->state = ATOMS_APP_STATE_LOADED;
    app->pid = next_pid++;

    if (app->on_init) {
        extern uint32_t g_current_creating_pid;
        g_current_creating_pid = app->pid;
        uint32_t window_id = 0;
        bwe_error_t err = app->on_init(&window_id);
        g_current_creating_pid = 0;
        if (err != BWE_SUCCESS) {
            app->state = ATOMS_APP_STATE_CLOSED;
            app->pid = 0;
            app_log("ERR", "App initialization failed!");
            return err;
        }

        app->main_window_id = window_id;
        if (window_id > 0) {
            ATOMS_RegisterWindowOwner(app->app_id, window_id);
        }
    }

    app->state = ATOMS_APP_STATE_RUNNING;
    g_focused_app_id = app->app_id;
    if (app->main_window_id) BOS_SetFocus(app->main_window_id);

    app_log_id("INFO", "App Launched & Running. Window", app->main_window_id);
    return BWE_SUCCESS;
}

bwe_error_t ATOMS_StopApplication(uint32_t app_id) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return BWE0001;
    if (app->state == ATOMS_APP_STATE_CLOSED || app->state == ATOMS_APP_STATE_STOPPED) return BWE_SUCCESS;

    app_log("INFO", "Stopping Application:");
    display_print("  Name: "); display_print(app->name); display_print("\n");

    app->state = ATOMS_APP_STATE_CLOSING;

    if (app->on_exit) {
        app->on_exit();
    }

    for (uint32_t w = 0; w < app->window_count; w++) {
        if (app->window_ids[w] != 0) {
            BOS_DestroySurface(app->window_ids[w]);
            app->window_ids[w] = 0;
        }
    }
    if (app->main_window_id != 0) {
        BOS_DestroySurface(app->main_window_id);
        app->main_window_id = 0;
    }

    app->state = ATOMS_APP_STATE_STOPPED;
    app->window_count = 0;
    app->pid = 0;
    if (g_focused_app_id == app->app_id) g_focused_app_id = 0;

    app_log("INFO", "App Stopped. All Owned Windows Released.");
    return BWE_SUCCESS;
}

bwe_error_t ATOMS_StopApplicationByWindow(uint32_t window_id) {
    ATOMS_Application* app = ATOMS_GetApplicationByWindow(window_id);
    if (app) {
        return ATOMS_StopApplication(app->app_id);
    }
    return BOS_DestroySurface(window_id);
}

bwe_error_t ATOMS_SetApplicationState(uint32_t app_id, ATOMS_AppState new_state) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return BWE0001;
    app->state = new_state;
    return BWE_SUCCESS;
}

ATOMS_Application* ATOMS_GetApplication(uint32_t app_id) {
    for (uint32_t i = 0; i < ATOMS_MAX_APPS; i++) {
        if (app_pool[i].app_id == app_id) return &app_pool[i];
    }
    return 0;
}

ATOMS_Application* ATOMS_GetApplicationByPID(uint32_t pid) {
    for (uint32_t i = 0; i < ATOMS_MAX_APPS; i++) {
        if (app_pool[i].pid == pid && app_pool[i].state != ATOMS_APP_STATE_CLOSED) {
            return &app_pool[i];
        }
    }
    return 0;
}

ATOMS_Application* ATOMS_GetApplicationByWindow(uint32_t window_id) {
    for (uint32_t i = 0; i < ATOMS_MAX_APPS; i++) {
        if (app_pool[i].state == ATOMS_APP_STATE_RUNNING || app_pool[i].state == ATOMS_APP_STATE_BACKGROUND) {
            if (app_pool[i].main_window_id == window_id) return &app_pool[i];
            for (uint32_t w = 0; w < app_pool[i].window_count; w++) {
                if (app_pool[i].window_ids[w] == window_id) return &app_pool[i];
            }
        }
    }
    return 0;
}

uint32_t ATOMS_GetActiveAppCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_APPS; i++) {
        if (app_pool[i].state == ATOMS_APP_STATE_RUNNING || app_pool[i].state == ATOMS_APP_STATE_BACKGROUND) {
            count++;
        }
    }
    return count;
}

void ATOMS_RegisterWindowOwner(uint32_t app_id, uint32_t window_id) {
    ATOMS_Application* app = ATOMS_GetApplication(app_id);
    if (!app) return;
    if (app->window_count < 16) {
        app->window_ids[app->window_count++] = window_id;
    }
    if (app->main_window_id == 0) app->main_window_id = window_id;
}

void ATOMS_UnregisterWindowOwner(uint32_t window_id) {
    ATOMS_Application* app = ATOMS_GetApplicationByWindow(window_id);
    if (!app) return;
    for (uint32_t i = 0; i < app->window_count; i++) {
        if (app->window_ids[i] == window_id) {
            for (uint32_t j = i; j < app->window_count - 1; j++) {
                app->window_ids[j] = app->window_ids[j + 1];
            }
            app->window_count--;
            break;
        }
    }
}

uint32_t ATOMS_GetFocusedAppID(void) {
    return g_focused_app_id;
}

// BOS Legacy Wrappers
void BOS_AppManager_Init(void) { ATOMS_AppManager_Init(); }
bwe_error_t BOS_RegisterApplication(const char* name, const char* version, BWE_AppInitFunc on_init, BWE_AppExitFunc on_exit, uint32_t* out_app_id) {
    return ATOMS_RegisterApplication(name, version, 0, on_init, on_exit, 0, out_app_id);
}
bwe_error_t BOS_StartApplication(uint32_t app_id) { return ATOMS_StartApplication(app_id); }
bwe_error_t BOS_StopApplication(uint32_t app_id) { return ATOMS_StopApplication(app_id); }
bwe_error_t BOS_StopApplicationByWindow(uint32_t window_id) { return ATOMS_StopApplicationByWindow(window_id); }
BOS_Application* BOS_GetApplication(uint32_t app_id) { return ATOMS_GetApplication(app_id); }
