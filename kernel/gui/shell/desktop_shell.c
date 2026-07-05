#include "desktop_shell.h"
#include "taskbar.h"
#include "desktop_icons.h"
#include "kernel/gui/desktop/desktop_surface.h"
#include "kernel/gui/startmenu/start_menu.h"
#include "kernel/gui/apps/app_registry.h"

// Dummy App headers
#include "kernel/apps/terminal/terminal_app.h"
#include "kernel/apps/music/music_app.h"
#include "kernel/apps/settings/settings_app.h"
#include "kernel/apps/files/files_app.h"
#include "kernel/apps/calculator/calculator_app.h"

static bool is_initialized = false;

// Mock callbacks for demo icons
static void on_terminal_click(void) {}
static void on_music_click(void) {}
static void on_settings_click(void) {}
static void on_files_click(void) {}

bool desktop_shell_init(int screen_width, int screen_height) {
    if (is_initialized) return false;

    // 0. Initialize App Registry
    app_registry_init();
    app_register(1, "Terminal", "System", 1, terminal_app_launch);
    app_register(2, "Music Player", "Media", 2, music_app_launch);
    app_register(3, "Settings", "System", 3, settings_app_launch);
    app_register(4, "File Manager", "System", 4, files_app_launch);
    app_register(5, "Calculator", "Accessories", 5, calculator_app_launch);

    // 1. Initialize root desktop surface
    if (!desktop_init(screen_width, screen_height)) return false;
    struct BOSSurface* root = desktop_get_root();

    // 1.5 Initialize Start Menu (attaches to root)
    start_menu_init(root);

    // 2. Initialize Taskbar (anchors to bottom, sets up Start Button internally)
    if (!taskbar_init(root, screen_width, screen_height)) return false;

    // 3. Initialize Desktop Icons
    desktop_icons_init(root);
    
    // Add Demo Icons (Now launching from registry via wrapper or directly)
    desktop_icon_add(20, 20, "Terminal", 1, terminal_app_launch);
    desktop_icon_add(20, 100, "Music", 2, music_app_launch);
    desktop_icon_add(20, 180, "Settings", 3, settings_app_launch);
    desktop_icon_add(20, 260, "Files", 4, files_app_launch);

    is_initialized = true;
    return true;
}

void desktop_shell_render(const BVRect* clip) {
    if (!is_initialized) return;
    
    struct BOSSurface* root = desktop_get_root();
    if (!root) return;
    
    // Desktop Background
    theme_draw_desktop(root, clip);
    
    // Desktop Icons
    desktop_icons_draw(clip);
    
    // Note: Taskbar is a child surface, so the compositor will naturally ask it to draw itself
    // if we hooked it up properly in compose_tree. If not, we would call taskbar_draw(clip) here.
    // Assuming taskbar_draw is called by compositor when the taskbar surface is processed.
}

bool desktop_shell_hit_test(int screen_x, int screen_y, int buttons) {
    if (!is_initialized) return false;
    
    // Shell elements get first dibs on input (Layer ordering)
    
    // 0. Start Menu (Top Most)
    if (start_menu_is_visible()) {
        if (start_menu_hit_test(screen_x, screen_y, buttons)) {
            return true;
        }
    }
    
    // 1. Taskbar (includes Start Button)
    if (taskbar_hit_test(screen_x, screen_y, buttons)) {
        return true;
    }
    
    // 2. Desktop Icons
    if (desktop_icons_hit_test(screen_x, screen_y, buttons)) {
        return true; // Technically handled, though clicking desktop bg might clear selection
    }
    
    // 3. Desktop Background (Clicking empties selection)
    struct BOSSurface* root = desktop_get_root();
    if (root) {
        // If it falls through windows, it hits the desktop
        // E.g. clear icon selection if clicking empty space
    }
    
    return false;
}

void desktop_shell_shutdown(void) {
    is_initialized = false;
    // Tear down surfaces if needed, but usually Window Manager shutdown handles the tree.
}

bool desktop_launch_application(int app_id) {
    return app_launch(app_id);
}
