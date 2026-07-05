#ifndef GUI_APP_REGISTRY_H
#define GUI_APP_REGISTRY_H

#include <stdbool.h>

#define MAX_APPLICATIONS 32
#define APP_NAME_LEN 64
#define APP_CAT_LEN 32

typedef void (*AppLaunchCallback)(void);

typedef struct {
    int id;
    char name[APP_NAME_LEN];
    char category[APP_CAT_LEN];
    int icon_id;
    AppLaunchCallback launch;
    bool visible_in_start_menu;
    bool enabled;
} Application;

void app_registry_init(void);

// Register an application
bool app_register(int id, const char* name, const char* category, int icon_id, AppLaunchCallback launch_cb);

// Unregister
void app_unregister(int id);

// Find an app by ID
Application* app_find(int id);

// Launch an app
bool app_launch(int id);

// Get the registry for enumeration
Application* app_list(void);
int app_count(void);

#endif // GUI_APP_REGISTRY_H
