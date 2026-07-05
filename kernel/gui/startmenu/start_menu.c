#include "start_menu.h"
#include "kernel/gui/apps/app_registry.h"
#include "kernel/gui/theme/theme_engine.h"
#include "kernel/gui/compositor/compositor.h"
#include "kernel/gui/shell/taskbar.h"
#include <stddef.h>

#define START_MENU_WIDTH 300
#define START_MENU_HEIGHT 450
#define MENU_ITEM_HEIGHT 40

static struct BOSSurface* start_menu_surface = NULL;
static StartMenuState current_state = START_MENU_HIDDEN;
static int hovered_item_index = -1;
static struct BOSSurface* desktop_ref = NULL;

void start_menu_init(struct BOSSurface* desktop_root) {
    if (start_menu_surface) return;
    
    desktop_ref = desktop_root;
    start_menu_surface = surface_create(START_MENU_WIDTH, START_MENU_HEIGHT);
    if (!start_menu_surface) return;
    
    // Position it just above the taskbar, on the left
    start_menu_surface->x = 0;
    start_menu_surface->y = desktop_root->height - TASKBAR_HEIGHT - START_MENU_HEIGHT;
    start_menu_surface->visible = false;
    current_state = START_MENU_HIDDEN;
    
    surface_add_child(desktop_root, start_menu_surface);
}

static void start_menu_bring_to_front(void) {
    if (!desktop_ref || !start_menu_surface) return;
    // Detach and reattach at the head of the list so it renders on top
    surface_remove_child(desktop_ref, start_menu_surface);
    surface_add_child(desktop_ref, start_menu_surface);
}

void start_menu_show(void) {
    if (current_state == START_MENU_VISIBLE) return;
    
    start_menu_bring_to_front();
    start_menu_surface->visible = true;
    current_state = START_MENU_VISIBLE;
    hovered_item_index = -1;
    
    compositor_invalidate_surface(start_menu_surface);
}

void start_menu_hide(void) {
    if (current_state == START_MENU_HIDDEN) return;
    
    compositor_invalidate_surface(start_menu_surface);
    start_menu_surface->visible = false;
    current_state = START_MENU_HIDDEN;
}

void start_menu_toggle(void) {
    if (current_state == START_MENU_VISIBLE) {
        start_menu_hide();
    } else {
        start_menu_show();
    }
}

void start_menu_render(const BVRect* clip) {
    if (current_state == START_MENU_HIDDEN || !start_menu_surface) return;
    
    theme_draw_start_menu(start_menu_surface, clip);
    
    Application* apps = app_list();
    int count = app_count();
    
    int y_offset = 50; // Leave space for header
    int valid_app_index = 0;
    
    for (int i = 0; i < MAX_APPLICATIONS; i++) {
        if (apps[i].enabled && apps[i].visible_in_start_menu) {
            bool is_hovered = (valid_app_index == hovered_item_index);
            
            theme_draw_menu_item(start_menu_surface, 
                                 10, y_offset, 
                                 START_MENU_WIDTH - 20, MENU_ITEM_HEIGHT, 
                                 apps[i].name, apps[i].icon_id, is_hovered, clip);
                                 
            y_offset += MENU_ITEM_HEIGHT;
            valid_app_index++;
        }
    }
    
    // Draw footer (Shutdown)
    bool is_shutdown_hovered = (hovered_item_index == 999);
    theme_draw_menu_item(start_menu_surface, 
                         10, START_MENU_HEIGHT - 50, 
                         START_MENU_WIDTH - 20, MENU_ITEM_HEIGHT, 
                         "Shutdown", 0, is_shutdown_hovered, clip);
}

bool start_menu_hit_test(int screen_x, int screen_y, int buttons) {
    if (current_state == START_MENU_HIDDEN || !start_menu_surface) return false;
    
    // Check if click is inside the start menu
    bool is_inside = (screen_x >= start_menu_surface->x && 
                      screen_x < start_menu_surface->x + start_menu_surface->width &&
                      screen_y >= start_menu_surface->y && 
                      screen_y < start_menu_surface->y + start_menu_surface->height);
                      
    if (!is_inside) {
        if (buttons & 1) { // Left click outside
            start_menu_hide();
        }
        return false;
    }
    
    int local_y = screen_y - start_menu_surface->y;
    int old_hover = hovered_item_index;
    
    // Header is 0-50, Footer is START_MENU_HEIGHT-50 to START_MENU_HEIGHT
    if (local_y >= 50 && local_y < START_MENU_HEIGHT - 50) {
        int index = (local_y - 50) / MENU_ITEM_HEIGHT;
        if (index < app_count()) {
            hovered_item_index = index;
            if (buttons & 1) { // Left click launch
                Application* apps = app_list();
                int valid_idx = 0;
                for (int i = 0; i < MAX_APPLICATIONS; i++) {
                    if (apps[i].enabled && apps[i].visible_in_start_menu) {
                        if (valid_idx == index) {
                            app_launch(apps[i].id);
                            start_menu_hide();
                            break;
                        }
                        valid_idx++;
                    }
                }
            }
        } else {
            hovered_item_index = -1;
        }
    } else if (local_y >= START_MENU_HEIGHT - 50) {
        hovered_item_index = 999; // Magic number for shutdown
        if (buttons & 1) {
            // Initiate shutdown sequence
        }
    } else {
        hovered_item_index = -1;
    }
    
    if (old_hover != hovered_item_index) {
        compositor_invalidate_surface(start_menu_surface);
    }
    
    return true; // Handled
}

bool start_menu_is_visible(void) {
    return current_state == START_MENU_VISIBLE;
}
