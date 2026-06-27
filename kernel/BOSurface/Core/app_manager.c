// ============================================================
// BOSurface Application Manager — Phase 9
// ============================================================
// Implements:
//   - Application Registration (built-in apps)
//   - Application Lifecycle (Start, Stop, Cleanup)
//   - Window-to-Application lookup
// ============================================================

#include "app_manager.h"
#include "../../display/display.h"

// ============================================================
// Static Application Pool
// ============================================================
static BOS_Application app_pool[BWE_MAX_APPS];
static uint32_t next_app_id = 1;
static uint32_t next_pid = 100; // PIDs start at 100

// ============================================================
// Internal Logging
// ============================================================
static void app_log(const char* level, const char* msg) {
    display_print("[APP_");
    display_print(level);
    display_print("] ");
    display_print(msg);
    display_print("\n");
}

static void app_log_id(const char* level, const char* msg, uint32_t id) {
    display_print("[APP_");
    display_print(level);
    display_print("] ");
    display_print(msg);
    display_print(" #");
    display_print_dec(id);
    display_print("\n");
}

// ============================================================
// BOS_AppManager_Init
// ============================================================
void BOS_AppManager_Init(void) {
    for (uint32_t i = 0; i < BWE_MAX_APPS; i++) {
        app_pool[i].app_id = 0;
        app_pool[i].state = BWE_APP_STATE_CLOSED;
        app_pool[i].pid = 0;
        app_pool[i].main_window_id = 0;
        app_pool[i].on_init = 0;
        app_pool[i].on_exit = 0;
        app_pool[i].name[0] = '\0';
        app_pool[i].version[0] = '\0';
    }
    next_app_id = 1;
    next_pid = 100;
    app_log("INFO", "Application Manager Initialized");
}

// ============================================================
// Internal: simple string copy (no libc)
// ============================================================
static void app_strcpy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

// ============================================================
// BOS_RegisterApplication
// ============================================================
bwe_error_t BOS_RegisterApplication(const char* name, const char* version,
                                     BWE_AppInitFunc on_init, BWE_AppExitFunc on_exit,
                                     uint32_t* out_app_id) {
    if (!name || !on_init) return BWE0001;

    // Find free slot
    BOS_Application* app = 0;
    for (uint32_t i = 0; i < BWE_MAX_APPS; i++) {
        if (app_pool[i].state == BWE_APP_STATE_CLOSED && app_pool[i].app_id == 0) {
            app = &app_pool[i];
            break;
        }
    }
    if (!app) return BWE0004; // No free slots

    app->app_id = next_app_id++;
    app_strcpy(app->name, name, 64);
    app_strcpy(app->version, version ? version : "1.0", 16);
    app->on_init = on_init;
    app->on_exit = on_exit;
    app->state = BWE_APP_STATE_CLOSED;
    app->pid = 0;
    app->main_window_id = 0;

    if (out_app_id) *out_app_id = app->app_id;

    app_log("INFO", "Registered Application:");
    display_print("  Name: ");
    display_print(name);
    display_print("\n");

    return BWE_SUCCESS;
}

// ============================================================
// BOS_StartApplication
// ============================================================
bwe_error_t BOS_StartApplication(uint32_t app_id) {
    BOS_Application* app = BOS_GetApplication(app_id);
    if (!app) return BWE0001;

    // Already running? Just bring window to front
    if (app->state == BWE_APP_STATE_RUNNING) {
        app_log_id("INFO", "App already running, focusing window", app->main_window_id);
        BOS_SetFocus(app->main_window_id);
        return BWE_SUCCESS;
    }

    app_log("INFO", "Starting Application:");
    display_print("  Name: ");
    display_print(app->name);
    display_print("\n");

    // Transition: CLOSED -> STARTING
    app->state = BWE_APP_STATE_STARTING;
    app->pid = next_pid++;

    // Call the app's init function — it creates the main window
    extern uint32_t g_current_creating_pid;
    g_current_creating_pid = app->pid;
    uint32_t window_id = 0;
    bwe_error_t err = app->on_init(&window_id);
    g_current_creating_pid = 0;
    if (err != BWE_SUCCESS) {
        app->state = BWE_APP_STATE_CLOSED;
        app->pid = 0;
        app_log("ERR", "App init failed!");
        return err;
    }

    app->main_window_id = window_id;

    // Transition: STARTING -> RUNNING
    app->state = BWE_APP_STATE_RUNNING;
    
    // Focus the new window
    BOS_SetFocus(window_id);

    app_log_id("INFO", "App Running. Window", window_id);

    return BWE_SUCCESS;
}

// ============================================================
// BOS_StopApplication
// ============================================================
bwe_error_t BOS_StopApplication(uint32_t app_id) {
    BOS_Application* app = BOS_GetApplication(app_id);
    if (!app) return BWE0001;
    if (app->state == BWE_APP_STATE_CLOSED) return BWE_SUCCESS; // Already closed

    app_log("INFO", "Stopping Application:");
    display_print("  Name: ");
    display_print(app->name);
    display_print("\n");

    // Transition: RUNNING -> CLOSING
    app->state = BWE_APP_STATE_CLOSING;

    // Call app's exit handler for cleanup
    if (app->on_exit) {
        app->on_exit();
    }

    // Destroy the main window
    if (app->main_window_id != 0) {
        BOS_CloseSurface(app->main_window_id);
    }

    // Transition: CLOSING -> CLOSED
    app->state = BWE_APP_STATE_CLOSED;
    app->main_window_id = 0;
    app->pid = 0;

    // Reset the slot so it can be started again
    // Keep app_id, name, version, on_init, on_exit intact — re-launchable!

    app_log("INFO", "App Stopped. Resources Released.");

    return BWE_SUCCESS;
}

// ============================================================
// BOS_StopApplicationByWindow — Close button integration
// ============================================================
bwe_error_t BOS_StopApplicationByWindow(uint32_t window_id) {
    for (uint32_t i = 0; i < BWE_MAX_APPS; i++) {
        if (app_pool[i].state == BWE_APP_STATE_RUNNING &&
            app_pool[i].main_window_id == window_id) {
            return BOS_StopApplication(app_pool[i].app_id);
        }
    }
    // No app owns this window — just close the surface directly
    return BOS_CloseSurface(window_id);
}

// ============================================================
// BOS_GetApplication
// ============================================================
BOS_Application* BOS_GetApplication(uint32_t app_id) {
    for (uint32_t i = 0; i < BWE_MAX_APPS; i++) {
        if (app_pool[i].app_id == app_id) {
            return &app_pool[i];
        }
    }
    return 0;
}
