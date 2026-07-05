#ifndef DESKTOP_SHELL_H
#define DESKTOP_SHELL_H

#include "kernel/wm/bwe/include/bwe.h"

// Shell Lifecycle & Manager
bwe_error_t Desktop_Shell_Initialize(void);

// Notification Subsystem
bwe_error_t Shell_ShowNotification(const char* title, const char* message, uint32_t duration_ms);

// Wallpaper API
struct BOSSurface;
void desktop_set_wallpaper(struct BOSSurface* surface);
struct BOSSurface* desktop_get_wallpaper(void);
void desktop_refresh_background(void);

// Wallpaper Transition APIs
void desktop_set_wallpaper_transition(struct BOSSurface* old_surface, struct BOSSurface* new_surface);
void desktop_set_wallpaper_alpha(uint32_t alpha); // 0 to 255
void desktop_end_wallpaper_transition(void);

// Wallpaper Render Helper
void Shell_DrawWallpaper(const BVFramebuffer* fb, const BWE_Rect* clip);

// Application Registry Entry Structure
typedef struct {
    uint32_t    app_id;
    const char* display_name;
    uint32_t    icon_id;
    bwe_error_t (*launch_callback)(uint32_t* out_win_id);
    const char* win_class;
    const char* category;
} ShellAppEntry;

// App Registry Accessors
bwe_error_t Shell_RegisterApp(const char* name, bwe_error_t (*launch_cb)(uint32_t*), const char* category, uint32_t* out_id);
bwe_error_t Shell_LaunchApp(uint32_t app_id, uint32_t* out_win_id);
uint32_t    Shell_GetAppCount(void);
ShellAppEntry* Shell_GetAppEntry(uint32_t index);

#endif // DESKTOP_SHELL_H
