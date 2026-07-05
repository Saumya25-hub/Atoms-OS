#include "kernel/gui/theme/theme_engine.h"
#include "kernel/gui/compositor/compositor.h"
#include "kernel/gui/events/gui_event.h"
#include "kernel/gui/startmenu/start_menu.h"

// 0 = Normal, 1 = Hover, 2 = Pressed
static int current_state = 0;
static struct BOSSurface* parent_taskbar = NULL;

void start_button_init(struct BOSSurface* taskbar_surface) {
    parent_taskbar = taskbar_surface;
    current_state = 0;
}

void start_button_draw(const BVRect* clip) {
    if (!parent_taskbar) return;
    
    int start_y = (parent_taskbar->height - START_BUTTON_HEIGHT) / 2;
    theme_draw_start_button(parent_taskbar, START_BUTTON_MARGIN, start_y, START_BUTTON_WIDTH, START_BUTTON_HEIGHT, current_state, clip);
}

void start_button_hit_test(int local_x, int local_y, int buttons) {
    if (!parent_taskbar) return;
    
    int start_y = (parent_taskbar->height - START_BUTTON_HEIGHT) / 2;
    bool is_inside = (local_x >= START_BUTTON_MARGIN && local_x < START_BUTTON_MARGIN + START_BUTTON_WIDTH &&
                      local_y >= start_y && local_y < start_y + START_BUTTON_HEIGHT);
                      
    int old_state = current_state;
    
    if (is_inside) {
        if (buttons & 1) { // Left click
            current_state = 2; // Pressed
        } else {
            if (old_state == 2) {
                // Released inside! Trigger Start Menu event
                GUIEvent ev;
                ev.type = GUI_EVENT_STARTMENU_OPEN;
                ev.target_surface_id = 0; // Shell event
                
                // Directly trigger start menu toggle
                start_menu_toggle();
            }
            current_state = 1; // Hover
        }
    } else {
        current_state = 0; // Normal
    }
    
    if (old_state != current_state) {
        // Only invalidate the button's rectangle, not the whole taskbar
        BVRect btn_rect = {
            parent_taskbar->x + START_BUTTON_MARGIN, 
            parent_taskbar->y + start_y, 
            START_BUTTON_WIDTH, 
            START_BUTTON_HEIGHT
        };
        compositor_invalidate_rect(&btn_rect);
        // Redraw will be triggered via compositor since we invalidated the screen area.
        // During the compose pass, the taskbar will be asked to redraw that clip rect.
    }
}
