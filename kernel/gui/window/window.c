#include "window.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/gui/compositor/compositor.h"
#include "kernel/gui/desktop/desktop_surface.h"
#include "kernel/gui/theme/theme_engine.h"
#include <string.h>
#include "kernel/gui/controls/control.h"
#include "kernel/gui/controls/panel.h"

static uint32_t next_window_id = 1;
static struct BOSWindow* window_list_head = NULL;
static struct BOSWindow* focused_window = NULL;

static void _strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    if (i < n) dest[i] = '\0';
    else if (n > 0) dest[n - 1] = '\0';
}

struct BOSWindow* window_create(const char* title, int width, int height, uint32_t owner_pid) {
    struct BOSWindow* win = (struct BOSWindow*)kmalloc(sizeof(struct BOSWindow));
    if (!win) return NULL;

    win->id = next_window_id++;
    if (title) _strncpy(win->title, title, sizeof(win->title));
    else win->title[0] = '\0';
    
    win->owner_pid = owner_pid;
    win->state = WINDOW_STATE_HIDDEN;
    
    win->surface = surface_create(width, height);
    if (!win->surface) {
        kfree(win);
        return NULL;
    }
    win->surface->owner_pid = owner_pid; // Sync ownership
    
    gui_event_queue_init(&win->event_queue);
    
    win->min_width = 100;
    win->min_height = 100;
    win->max_width = 4096;
    win->max_height = 4096;
    
    win->flags = 0;
    win->focused = false;

    // Create root panel covering the client area
    // For simplicity, we just use control_init since it's an abstract panel essentially, 
    // or we can use panel_create() if we include panel.h.
    win->root_panel = (struct BOSControl*)panel_create();
    if (win->root_panel) {
        win->root_panel->window = win;
        panel_set_transparent((BOSPanel*)win->root_panel, true);
        control_set_position(win->root_panel, 0, THEME_TITLEBAR_HEIGHT);
        control_set_size(win->root_panel, width, height - THEME_TITLEBAR_HEIGHT);
    }
    win->hovered_control = NULL;
    win->focused_control = NULL;

    struct BOSSurface* desktop = desktop_get_root();
    if (desktop) {
        surface_add_child(desktop, win->surface);
    }

    win->next = window_list_head;
    window_list_head = win;
    
    // Draw initial decorations
    theme_draw_window_frame(win->surface, win->title, false, NULL);
    theme_draw_client_background(win->surface, false, NULL);

    return win;
}

void window_destroy(struct BOSWindow* window) {
    if (!window) return;
    window->state = WINDOW_STATE_DESTROYED;
    
    if (window_list_head == window) {
        window_list_head = window->next;
    } else {
        struct BOSWindow* curr = window_list_head;
        while (curr && curr->next != window) curr = curr->next;
        if (curr) curr->next = window->next;
    }

    if (focused_window == window) focused_window = NULL;

    if (window->root_panel) {
        control_destroy_recursive(window->root_panel);
        window->root_panel = NULL;
    }

    if (window->surface) {
        compositor_invalidate_surface(window->surface);
        surface_destroy(window->surface);
    }
    kfree(window);
}

void window_show(struct BOSWindow* window) {
    if (!window) return;
    window->state = WINDOW_STATE_NORMAL;
    surface_set_visible(window->surface, true);
    compositor_invalidate_surface(window->surface);
}

void window_hide(struct BOSWindow* window) {
    if (!window) return;
    window->state = WINDOW_STATE_HIDDEN;
    surface_set_visible(window->surface, false);
    compositor_invalidate_surface(window->surface);
}

void window_move(struct BOSWindow* window, int x, int y) {
    if (!window) return;
    compositor_invalidate_surface(window->surface);
    surface_set_position(window->surface, x, y);
    compositor_invalidate_surface(window->surface);
}

void window_resize(struct BOSWindow* window, int width, int height) {
    if (!window) return;
    if (width < window->min_width) width = window->min_width;
    if (width > window->max_width) width = window->max_width;
    if (height < window->min_height) height = window->min_height;
    if (height > window->max_height) height = window->max_height;

    if (width == window->surface->width && height == window->surface->height) return;

    uint32_t* new_fb = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    if (!new_fb) return;

    uint32_t* old_fb = window->surface->framebuffer;
    compositor_invalidate_surface(window->surface);

    window->surface->framebuffer = new_fb;
    window->surface->width = width;
    window->surface->height = height;
    if (old_fb) kfree(old_fb);

    // Redraw frame on resize
    theme_draw_window_frame(window->surface, window->title, window->focused, NULL);
    theme_draw_client_background(window->surface, window->focused, NULL);

    compositor_invalidate_surface(window->surface);
    
    // Post resize event
    GUIEvent ev;
    ev.type = GUI_EVENT_WINDOW_RESIZE;
    ev.target_surface_id = window->surface->unique_id;
    ev.data.resize.width = width;
    ev.data.resize.height = height;
    window_post_event(window, &ev);
}

void window_focus(struct BOSWindow* window) {
    if (!window) return;
    if (focused_window && focused_window != window) {
        focused_window->focused = false;
        theme_draw_window_frame(focused_window->surface, focused_window->title, false, NULL);
        compositor_invalidate_surface(focused_window->surface); 
    }
    focused_window = window;
    window->focused = true;
    
    window_bring_to_front(window);
    
    theme_draw_window_frame(window->surface, window->title, true, NULL);
    compositor_invalidate_surface(window->surface);
}

void window_close(struct BOSWindow* window) {
    if (!window) return;
    GUIEvent ev;
    ev.type = GUI_EVENT_WINDOW_CLOSE;
    ev.target_surface_id = window->surface->unique_id;
    window_post_event(window, &ev);
    // Real destruction happens either here or in process cleanup
}

void window_invalidate(struct BOSWindow* window) {
    if (!window) return;
    compositor_invalidate_surface(window->surface);
}

void window_redraw(struct BOSWindow* window) {
    if (!window) return;
    
    // Draw background/frame
    theme_draw_window_frame(window->surface, window->title, window->focused, NULL);
    theme_draw_client_background(window->surface, window->focused, NULL);
    
    // Draw controls
    if (window->root_panel && window->root_panel->paint) {
        window->root_panel->paint(window->root_panel, window->surface, NULL);
    }
    
    GUIEvent ev;
    ev.type = GUI_EVENT_PAINT;
    ev.target_surface_id = window->surface->unique_id;
    window_post_event(window, &ev);
    window_invalidate(window);
}

void window_invalidate_rect(struct BOSWindow* window, int x, int y, int width, int height) {
    if (!window) return;
    
    BVRect rect = {x, y, width, height};
    
    // In a real compositor, we'd add 'rect' to the dirty region.
    // For now, we invalidate the entire window surface to keep it simple,
    // or we can use compositor_invalidate_surface_rect if it exists.
    // Let's just invalidate the whole surface since it's the safest fallback.
    compositor_invalidate_surface(window->surface);
}

void window_add_control(struct BOSWindow* window, struct BOSControl* control) {
    if (!window || !control || !window->root_panel) return;
    control_add_child(window->root_panel, control);
}

void window_process_events(struct BOSWindow* window) {
    if (!window) return;
    GUIEvent ev;
    while (gui_event_pop(&window->event_queue, &ev)) {
        // Find target control via hit testing for mouse events
        if (ev.type >= GUI_EVENT_MOUSE_MOVE && ev.type <= GUI_EVENT_DOUBLE_CLICK) {
            if (window->root_panel) {
                struct BOSControl* hit = control_hit_test(window->root_panel, ev.data.mouse.x, ev.data.mouse.y);
                if (hit && hit->handle_event) {
                    hit->handle_event(hit, &ev);
                }
            }
        } 
        else if (ev.type == GUI_EVENT_KEY_DOWN || ev.type == GUI_EVENT_KEY_UP) {
            if (window->focused_control && window->focused_control->handle_event) {
                window->focused_control->handle_event(window->focused_control, &ev);
            }
        }
    }
}



void window_bring_to_front(struct BOSWindow* window) {
    if (!window || !window->surface) return;
    struct BOSSurface* root = desktop_get_root();
    if (root && window->surface->parent == root) {
        surface_remove_child(root, window->surface);
        surface_add_child(root, window->surface);
    }
}

bool window_post_event(struct BOSWindow* window, const GUIEvent* event) {
    if (!window || !event) return false;
    return gui_event_push(&window->event_queue, event);
}

WindowHitTestResult window_hit_test(struct BOSWindow* window, int screen_x, int screen_y) {
    if (!window || !window->surface || !window->surface->visible) return HIT_OUTSIDE;
    
    int wx = window->surface->x;
    int wy = window->surface->y;
    int ww = window->surface->width;
    int wh = window->surface->height;
    
    if (screen_x < wx || screen_x >= wx + ww || screen_y < wy || screen_y >= wy + wh) {
        return HIT_OUTSIDE;
    }
    
    int lx = screen_x - wx; // Local X
    int ly = screen_y - wy; // Local Y
    
    // Close button check
    if (ly >= 4 && ly <= 4 + THEME_BUTTON_SIZE && 
        lx >= ww - THEME_BUTTON_SIZE - 4 && lx <= ww - 4) {
        return HIT_CLOSE_BUTTON;
    }
    
    // Borders
    bool left = lx < THEME_BORDER_WIDTH;
    bool right = lx >= ww - THEME_BORDER_WIDTH;
    bool top = ly < THEME_BORDER_WIDTH;
    bool bottom = ly >= wh - THEME_BORDER_WIDTH;
    
    if (top && left) return HIT_BORDER_TOPLEFT;
    if (top && right) return HIT_BORDER_TOPRIGHT;
    if (bottom && left) return HIT_BORDER_BOTTOMLEFT;
    if (bottom && right) return HIT_BORDER_BOTTOMRIGHT;
    if (left) return HIT_BORDER_LEFT;
    if (right) return HIT_BORDER_RIGHT;
    if (bottom) return HIT_BORDER_BOTTOM;
    if (top) return HIT_BORDER_TOP;
    
    if (ly < THEME_TITLEBAR_HEIGHT) return HIT_TITLEBAR;
    
    return HIT_CLIENT;
}

struct BOSWindow* window_get_at_point(int screen_x, int screen_y) {
    // Need to traverse Z-order from top to bottom
    // The surface tree stores children in order, so we traverse desktop children backwards
    struct BOSSurface* root = desktop_get_root();
    if (!root) return NULL;
    
    struct BOSSurface* top_hit_surface = NULL;
    struct BOSSurface* curr = root->children;
    while (curr) {
        if (curr->visible && screen_x >= curr->x && screen_x < curr->x + curr->width &&
            screen_y >= curr->y && screen_y < curr->y + curr->height) {
            // Since children list means newer/higher z-order is added at the head usually,
            // we need to be sure. Wait, `surface_add_child` prepends to head.
            // So `root->children` is the TOPMOST surface!
            top_hit_surface = curr;
            break; 
        }
        curr = curr->next;
    }
    
    if (!top_hit_surface) return NULL;
    
    // Find window that owns this surface
    struct BOSWindow* w = window_list_head;
    while (w) {
        if (w->surface == top_hit_surface) return w;
        w = w->next;
    }
    
    return NULL;
}
