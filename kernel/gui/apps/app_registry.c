#include "app_registry.h"
#include <string.h>

static Application registry[MAX_APPLICATIONS];
static int total_apps = 0;

static void _strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    if (i < n) dest[i] = '\0';
    else if (n > 0) dest[n - 1] = '\0';
}

void app_registry_init(void) {
    for (int i = 0; i < MAX_APPLICATIONS; i++) {
        registry[i].enabled = false;
    }
    total_apps = 0;
}

bool app_register(int id, const char* name, const char* category, int icon_id, AppLaunchCallback launch_cb) {
    if (total_apps >= MAX_APPLICATIONS) return false;
    
    // Check if exists
    for (int i = 0; i < MAX_APPLICATIONS; i++) {
        if (registry[i].enabled && registry[i].id == id) {
            return false; // Already registered
        }
    }

    // Find free slot
    for (int i = 0; i < MAX_APPLICATIONS; i++) {
        if (!registry[i].enabled) {
            registry[i].id = id;
            _strncpy(registry[i].name, name, APP_NAME_LEN);
            _strncpy(registry[i].category, category, APP_CAT_LEN);
            registry[i].icon_id = icon_id;
            registry[i].launch = launch_cb;
            registry[i].visible_in_start_menu = true;
            registry[i].enabled = true;
            total_apps++;
            return true;
        }
    }
    return false;
}

void app_unregister(int id) {
    for (int i = 0; i < MAX_APPLICATIONS; i++) {
        if (registry[i].enabled && registry[i].id == id) {
            registry[i].enabled = false;
            total_apps--;
            return;
        }
    }
}

Application* app_find(int id) {
    for (int i = 0; i < MAX_APPLICATIONS; i++) {
        if (registry[i].enabled && registry[i].id == id) {
            return &registry[i];
        }
    }
    return NULL;
}

bool app_launch(int id) {
    Application* app = app_find(id);
    if (app && app->launch) {
        app->launch();
        return true;
    }
    return false;
}

Application* app_list(void) {
    return registry;
}

int app_count(void) {
    return total_apps;
}
