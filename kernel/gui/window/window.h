#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include "kernel/gui/surface/surface.h"
#include "kernel/gui/events/gui_event.h"
#include <stdint.h>
#include <stdbool.h>

struct BOSControl;

typedef enum {
    WINDOW_STATE_NORMAL,
    WINDOW_STATE_HIDDEN,
    WINDOW_STATE_MINIMIZED,
    WINDOW_STATE_MAXIMIZED,
    WINDOW_STATE_FULLSCREEN,
    WINDOW_STATE_DESTROYED
} BOSWindowState;

typedef enum {
    HIT_OUTSIDE = 0,
    HIT_CLIENT,
    HIT_TITLEBAR,
    HIT_CLOSE_BUTTON,
    HIT_BORDER_LEFT,
    HIT_BORDER_RIGHT,
    HIT_BORDER_TOP,
    HIT_BORDER_BOTTOM,
    HIT_BORDER_TOPLEFT,
    HIT_BORDER_TOPRIGHT,
    HIT_BORDER_BOTTOMLEFT,
    HIT_BORDER_BOTTOMRIGHT
} WindowHitTestResult;

struct BOSWindow {
    uint32_t id;
    char title[64];
    uint32_t owner_pid; // Process ID that owns this window
    BOSWindowState state;
    
    struct BOSSurface* surface;
    GUIEventQueue event_queue;
    
    int min_width;
    int min_height;
    int max_width;
    int max_height;
    
    struct BOSControl* root_panel;
    struct BOSControl* hovered_control;
    struct BOSControl* focused_control;
    
    uint32_t flags;
    bool focused;
    
    struct BOSWindow* next; // Linked list of all windows
};

// Window Manager API
struct BOSWindow* window_create(const char* title, int width, int height, uint32_t owner_pid);
void window_destroy(struct BOSWindow* window);

void window_show(struct BOSWindow* window);
void window_hide(struct BOSWindow* window);

void window_move(struct BOSWindow* window, int x, int y);
void window_resize(struct BOSWindow* window, int width, int height);

void window_focus(struct BOSWindow* window);
void window_close(struct BOSWindow* window);

void window_invalidate(struct BOSWindow* window);
void window_invalidate_rect(struct BOSWindow* window, int x, int y, int width, int height);
void window_redraw(struct BOSWindow* window);

void window_add_control(struct BOSWindow* window, struct BOSControl* control);
void window_process_events(struct BOSWindow* window);

void window_bring_to_front(struct BOSWindow* window);

// Send event to window
bool window_post_event(struct BOSWindow* window, const GUIEvent* event);

// Determine which part of the window was clicked
WindowHitTestResult window_hit_test(struct BOSWindow* window, int screen_x, int screen_y);

// Globally fetch top-most window under coordinates
struct BOSWindow* window_get_at_point(int screen_x, int screen_y);

#endif // GUI_WINDOW_H
