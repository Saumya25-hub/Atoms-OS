# Application Registry

The Application Registry (`app_registry.c`) abstracts the concept of "installed software" from the graphical shell.

## Structure

```c
typedef struct {
    int id;
    char name[APP_NAME_LEN];
    char category[APP_CAT_LEN];
    int icon_id;
    AppLaunchCallback launch;
    bool visible_in_start_menu;
    bool enabled;
} Application;
```

It exposes `app_launch(id)`, which triggers the function pointer used to spawn the application's main window. Both the Desktop Icons and the Start Menu rely on this central registry to prevent hardcoding.
